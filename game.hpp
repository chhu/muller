// Manages the Game state.

struct CruncherResult_s {
	bool finished;
	EvalResult best;
	uint64_t evals;
	TimePoint ms_taken;
};

struct CruncherInstruction_s {
	Board position;
	bool white_to_move;
	bool mate_search;
	bool pos_score_enabled;
	int depth;
};


struct Game {
	Board initial;	// Position the game started with, doesn't change
	Board current;
	bool white_to_move; // next move for white?
	bool first_white;	// first move of this game from white?
	vector<uint16_t> move_history;  // WBWBWBWB... moves if first_white == true, otherwise BWBWBWBWBW...
	vector<Board> 	 board_history;	// same here
	vector<EvalResult> eval_history;// and here

	Game(const string StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
  		setFEN(StartFEN);
  		current = initial;
	}

	Game(Board init, bool wtm) {
		initial = current = init;
		white_to_move = first_white = wtm;
		last_search_ms = last_search_start = 0;
	}

	void setFEN(const string fen) {
		move_history.clear(); 
		board_history.clear();
		eval_history.clear();
		initial.clear();
/*
   A FEN string defines a particular position using only the ASCII character set.

   A FEN string contains six fields separated by a space. The fields are:

   1) Piece placement (from white's perspective). Each rank is described, starting
      with rank 8 and ending with rank 1. Within each rank, the contents of each
      square are described from file A through file H. Following the Standard
      Algebraic Notation (SAN), each piece is identified by a single letter taken
      from the standard English names. White pieces are designated using upper-case
      letters ("PNBRQK") whilst Black uses lowercase ("pnbrqk"). Blank squares are
      noted using digits 1 through 8 (the number of blank squares), and "/"
      separates ranks.

   2) Active color. "w" means white moves next, "b" means black.

   3) Castling availability. If neither side can castle, this is "-". Otherwise,
      this has one or more letters: "K" (White can castle kingside), "Q" (White
      can castle queenside), "k" (Black can castle kingside), and/or "q" (Black
      can castle queenside).

   4) En passant target square (in algebraic notation). If there's no en passant
      target square, this is "-". If a pawn has just made a 2-square move, this
      is the position "behind" the pawn. This is recorded only if there is a pawn
      in position to make an en passant capture, and if there really is a pawn
      that might have advanced two squares.

   5) Halfmove clock. This is the number of halfmoves since the last pawn advance
      or capture. This is used to determine if a draw can be claimed under the
      fifty-move rule.

   6) Fullmove number. The number of the full move. It starts at 1, and is
      incremented after Black's move.
*/

		unsigned char col, row, token;
		size_t idx;
		E_PIECE pc;
		uint16_t sq = 8 * 7; // A8
		std::istringstream ss(fen);
		ss >> std::noskipws;

		// 1. Piece placement
		while ((ss >> token) && !isspace(token)) {
			if (isdigit(token))
				sq += (token - '0'); // Advance the given number of files
			else if (token == '/')
				sq += 2 * -8;
			else if ((pc = charToPiece(token)) != P_EMPTY) {
				initial.insert(pc, sq);
				++sq;
			}
		}

		// 2. Active color
		ss >> token;
		white_to_move = (token == 'w');
		ss >> token;

		// 3. Castling availability. Compatible with 3 standards: Normal FEN standard,
		// Shredder-FEN that uses the letters of the columns on which the rooks began
		// the game instead of KQkq and also X-FEN standard that, in case of Chess960,
		// if an inner rook is associated with the castling right, the castling tag is
		// replaced by the file letter of the involved rook, as for the Shredder-FEN.
		while ((ss >> token) && !isspace(token)) {
			token = char(token);
			if (token == 'K')      set_bit32(initial.game_flags, W_CK_BIT);
			else if (token == 'Q') set_bit32(initial.game_flags, W_CQ_BIT);
			if (token == 'k')      set_bit32(initial.game_flags, B_CK_BIT);
			else if (token == 'q') set_bit32(initial.game_flags, B_CQ_BIT);
		}

		// 4. En passant square. Ignore if no pawn capture is possible
		if (   ((ss >> col) && (col >= 'a' && col <= 'h'))
		   && ((ss >> row) && (row == '3' || row == '6')))  {
		   initial.enpassant_square = (col - 'a') + (row - '1') * 8;
		}  else
		   initial.enpassant_square = 65;
		current = initial;
	}
	void execMove(Move m) {
		EvalResult er{};
		er.move = m;
		er.score = 0;
		execMove(er);
	}

	void execMove(EvalResult er) {
		if (board_history.size() == 0) {
			current = initial;
		}
		E_PIECE took;
		move_history.push_back(er.move);
		eval_history.push_back(er);
		current = current.move(er.move, took);
		board_history.push_back(current);
		white_to_move = !white_to_move;
	}

	// comfort of vector only possible in higher level game state where speed does not matter anymore
	vector<Move> getValidMoves() {
	    MoveArray moves;
	    uint16_t m = current.moves(white_to_move, moves);
	    current.removeInvalid(white_to_move, m, moves);
		vector<Move> result;
		for (int i = 0; i < m; i++)
			result.push_back(moves[i]);
		return result;
	}

	bool isValidMove(Move move) {
		auto moves = getValidMoves();
		return find(moves.begin(), moves.end(), move) != moves.end();
	}

	bool isMate() {
		if (!getValidMoves().empty())
			return false;
		return !isStaleMate();
	}

	bool isStaleMate() {
		if (!getValidMoves().empty())
			return false;
		white_to_move = !white_to_move;
		auto m = getValidMoves();
		white_to_move = !white_to_move;
		E_PIECE take;
        E_PIECE own_king = white_to_move ? W_KING : B_KING;
        for (auto move : m) {
        	current.move(move, take);
        	if (take == own_king)
        		return false;
        }
        return true;
	}

	// returns index in er or -1 if er is empty
	EvalResult selectMove(ExtendedEvalResult er) {
		EvalResult result{};
		if (er.size() == 0)
			return result;

		if (limits.mate_search) { // mate search
			sort(er.begin(), er.end(), greater<EvalResult>());
			return er[0];
		}

		// give castle bonus & penalize check for castling. Easier to penalize than to remove from valid moves
		for (auto &e : er) {
			Board c = current;
			if (e.move == w_o_o) {
				c.insert(W_KING, get_bitpos(0, 5));
				c.insert(W_KING, get_bitpos(0, 6));
				e.score += c.isCheck(true) ? -value[W_KING] : 10;
			} else if (e.move == w_o_o_o) {
				c.insert(W_KING, get_bitpos(0, 2));
				c.insert(W_KING, get_bitpos(0, 3));
				e.score += c.isCheck(true) ? -value[W_KING] : 10;
			} else if (e.move == b_o_o) {
				c.insert(B_KING, get_bitpos(7, 5));
				c.insert(B_KING, get_bitpos(7, 6));
				e.score += c.isCheck(false) ? -value[W_KING] : 10;
			} else if (e.move == b_o_o_o) {
				c.insert(B_KING, get_bitpos(7, 2));
				c.insert(B_KING, get_bitpos(7, 3));
				e.score += c.isCheck(false) ? -value[W_KING] : 10;
			}
		}

		// give pawn move bonus
		for (auto &e : er)
			if (current.getPiece(move_from(e.move)) == (white_to_move ? W_PAWN : B_PAWN))
				e.score += 2;			

		sort(er.begin(), er.end(), greater<EvalResult>());
		/*
		int n = 0;
		auto best_score = er[0].score; // er is sorted
		for (const auto e : er)
			if (e.score >= best_score - ENG_MOVE_DEVIATION) 
				n++; 
			else
				break;
		er.resize(n);
		*/
		if (!limits.pos_score_enabled) { // if only material score, add at least pos score for available moves and re-sort 
			for (auto &e : er) {
				uint16_t dummy[128];E_PIECE dummy_;
				Board c = current.move(e.move, dummy_);
				uint16_t n_wtm = c.moves(white_to_move, dummy);
				c.removeInvalid(white_to_move, n_wtm, dummy);
				e.score += n_wtm;
				n_wtm = c.moves(!white_to_move, dummy);
				c.removeInvalid(!white_to_move, n_wtm, dummy);
				e.score -= n_wtm;
				// favor takes if score is in front?
			}
			sort(er.begin(), er.end(), greater<EvalResult>());
		}
		// Try to avoid draw by taking less optimal moves, down to ENG_MOVE_DEVIATION
		int desired_i = checkRepetition();
		cout << "info moveselect num " << er.size() << " rep " << desired_i << endl;
		for (auto e : er)
			cout << current.move2str(e.move) << " : " << e.score << endl;
		return er[min(desired_i, int(er.size()-1))];
	}

	// Prints useful stuff.
	ExtendedEvalResult search(int depth, uint32_t &ms_taken) {
	    evals = checks = ab_cuts = 0;
	    auto t1 = steady_clock::now();
	    auto moves = getValidMoves();
	    auto m = moves.size();
	    ExtendedEvalResult results;
	    results.resize(m);
	    cout << "OMP PAR search with up to " << m << " threads W "<< white_to_move << endl;
		#pragma omp parallel for shared(results) schedule(dynamic,1)
	    for (uint i = 0; i < m; i ++) {
		    auto t11 = steady_clock::now();
	        E_PIECE took;
	        Board new_board = current.move(moves[i], get_pcidx(current.position, move_from(moves[i])), took);
	        results[i] = f_negamax(depth - 1, new_board, INT32_MIN + 1, INT32_MAX, !white_to_move, -1, 0);
	        results[i].score = -results[i].score;
	        results[i].move = results[i].lot[depth] = moves[i];
	        fixLOT(results[i]);
		    auto t22 = steady_clock::now();
		    /* Getting number of milliseconds as an integer. */
		    auto duration__ = duration<double,milli>(t22 - t11);
		    cout << "M"<<i<<":"<<duration__.count() << " " ;printMove(results[i]); cout <<endl;
	    }
	    sort(results.begin(), results.end(), greater<EvalResult>());
	    auto t2 = steady_clock::now();
	    /* Getting number of milliseconds as an integer. */
	    auto duration_ = duration<double,milli>(t2 - t1);
	    double speed = (double)evals / duration_.count();
	    if (results.size() == 0)
	    	cout << move_history.size() << ": " << (white_to_move?"W ":"B ") << " NO MOVE LEFT!\n";
	    else {
	    	cout << move_history.size() << ": " << (white_to_move?"W ":"B ") << "[" << depth << "] ";printMove(results[0].move);cout << " / SCORE: " << results[0].score << " / #EVALS: " << evals << "/ #CUTS: " << ab_cuts << " / #CHECKS: " << checks << " / MS: " << duration_.count() << " / EPMS: " << speed << " \n";
	    }
	    ms_taken = duration_.count();
	    return results;
	}

	// current MPI search state
	struct MoveSearchRequest_s {
		Move search_request;
		CruncherInstruction_s instruction;
		CruncherResult_s result;
		int rank;
		MPI_Request mpi_request;
	};
	list<MoveSearchRequest_s> searchq;

	void stopSearchMPI(bool call_process = false) {
		int x = 1;
		for (const auto& ms : searchq)
			if (ms.rank > 0)
    			auto r = MPI_Put((void *)&x, 1, MPI_INT, ms.rank, 0, 1, MPI_INT, eng_halt_win);
		for (auto& ms : searchq)
			if (ms.rank > 0)
		    	MPI_Wait(&ms.mpi_request, MPI_STATUSES_IGNORE);
		// remove all pending from Q#
		auto it = searchq.begin();
		while (it != searchq.end()) {
			auto &sr = *it;
			if (sr.rank == 0) {
				it = searchq.erase(it);
			} else
				++it;
		};

		if (call_process)
			processSearchQ();
	}

	ExtendedEvalResult last_search_result;
	TimePoint last_search_start;
	uint64_t last_search_ms;

	void startSearchMPI(int depth) {
	    stopSearchMPI(true); // discard result
	    ResetStats();
	    last_search_start = now();
	    auto moves = getValidMoves();
	    startSearchMPI(moves, depth);
	}

	void startSearchMPI(vector<Move> moves, int depth) {
	    auto m = moves.size();
	    last_search_result.resize(0);
	    //cout << "MPI search queue of " << m << " moves W: "<< white_to_move << endl;
	    for (uint i = 0; i < m; i ++) {
	        E_PIECE took;
	        Board new_board = current.move(moves[i], get_pcidx(current.position, move_from(moves[i])), took);
	        MoveSearchRequest_s sreq = {};
	        sreq.instruction.depth = depth - 1;
	        sreq.instruction.position = new_board;
	        sreq.instruction.white_to_move = !white_to_move;
	        sreq.instruction.mate_search = limits.mate_search;
	        sreq.instruction.pos_score_enabled = limits.pos_score_enabled;
	        sreq.rank = 0;
	        sreq.search_request = moves[i];
	        searchq.push_back(sreq);
	    }
	}

	// return true if q is empty
	bool processSearchQ() {
		vector<bool> busy_ranks(cpu_count, false);
		for (auto &sr : searchq) 
			busy_ranks[sr.rank] = true; 

		// Deploy loop
		for (auto &sr : searchq) {
			if (sr.rank > 0)
				continue;
			// find rank not busy
			auto it = find(busy_ranks.begin(), busy_ranks.end(), false);
			if (it == busy_ranks.end()) break; // all ranks busy
    		int idle_rank = it - busy_ranks.begin();
    		busy_ranks[idle_rank] = true;
    		sr.rank = idle_rank;
    		//cout << "GO FOR " << idle_rank << " " << moves_to_crunch - 1 << endl;
    		MPI_Send((void *) &sr.instruction, sizeof(sr.instruction), MPI_BYTE, idle_rank, 0, MPI_COMM_WORLD);
    		MPI_Irecv((void *) &sr.result, sizeof(CruncherResult_s), MPI_BYTE, idle_rank, 0, MPI_COMM_WORLD, &sr.mpi_request);
    	}

    	// Check at all non-idle ranks if they finished
    	auto it = searchq.begin();
    	while (it != searchq.end()) {
    		auto &sr = *it;
    		if (sr.rank > 0) {
	    		int done = 0;
	    		MPI_Test(&sr.mpi_request, &done, MPI_STATUS_IGNORE);
	    		if (done) {
	    			EvalResult result = sr.result.best;
	        		result.score = -result.score;
	        		result.move = result.lot[sr.instruction.depth+1] = sr.search_request;
	        		evals += sr.result.evals;
	        		fixLOT(result);
	        		if (limits.debug_mainline) {
	        			cout << "M"<<searchq.size()<< ": " << sr.result.evals / (sr.result.ms_taken+1) << " EPMS. ";printMove(result); cout <<endl;
	        		}
	        		last_search_result.push_back(result);
	        		it = searchq.erase(it);
	        		continue;
	    		}
    		}
   			++it;
    	}
    	if (searchq.size() == 0) {
    		last_search_ms = since(last_search_start);
    		//if (evals > 0) cout << "TOOK: " << last_search_ms << "ms, PERF: " <<  evals/(last_search_ms+1) << " e/ms.\n";
    		return true;
    	}
    	return false;
	}

	void fixLOT(EvalResult &r) {
		int d = MAX_DEPTH-1;
		Game g = Game(current, white_to_move);
		while(r.lot[d] == 0 && d > 0) d--;
	    for (int j = d; j > 0; j--) {
	    	auto &next_move = r.lot[j];
	    	if (g.isMate()) {
	    		next_move = 0;
	    		break;
	    	}
	    	if (g.isStaleMate()) {
	    		next_move = 0xFFFF;
	    		r.score = 0; // DRAW!
	    		break;
	    	}
	    	if (!g.isValidMove(next_move)) {
	    		r.lot[j-1] = 0xFFEE;
	    		break;
	    	}
			g.execMove(next_move);
	    }
	}

	// did current board appear in history more or equal than n times?
	uint32_t checkRepetition() {
		uint32_t count = 0;
		for (const auto& b : board_history)
			if (current == b)
				count++;
		return count > 0 ? count-1 : 0;
	} 

	void receiverLoop() {
		CruncherInstruction_s CruncherInstruction;
		CruncherResult_s CruncherResult;
		MPI_Request incoming_request;
		int flag = 0;
		while(1) {
			ResetStats();
			CruncherResult.finished = true;
			MPI_Irecv((void *)&CruncherInstruction, sizeof(CruncherInstruction), MPI_BYTE, 0, 0, MPI_COMM_WORLD, &incoming_request);
			do {
				MPI_Test(&incoming_request, &flag, MPI_STATUS_IGNORE);
				sleep_ms(1);
			} while (!flag);
			*engine_halt = 0;  // in case we were stopped
			limits.mate_search = CruncherInstruction.mate_search;
			limits.pos_score_enabled = CruncherInstruction.pos_score_enabled;
			//CruncherInstruction.position.print();
			auto t_start = now();
			CruncherResult.best = f_pvs(CruncherInstruction.depth, CruncherInstruction.position, INT32_MIN + 1, INT32_MAX, CruncherInstruction.white_to_move, -1, 0);
		    CruncherResult.ms_taken = since(t_start);
		    CruncherResult.evals = evals;
			CruncherResult.finished = *engine_halt;
		    MPI_Send((void *)&CruncherResult, sizeof(CruncherResult), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
		}
	}
};
