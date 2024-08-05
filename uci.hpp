// UCI interface, mostly copied from stockfish
typedef uint8_t Square; // bitpos
map<string,string> Options;

/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCIvalue(int32_t v) {

  stringstream ss;
  int VALUE_MATE = INT32_MAX / 2;
  if (abs(v) < INT32_MAX / 4)
      ss << "cp " << v;
  else
      ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

  return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCIsquare(Square s) {
  return std::string{ char('a' + get_file(s)), char('1' + get_rank(s)) };
}


/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

string UCImove(Game &g, Move m, bool chess960) {

  Square from = move_from(m);
  Square to = move_to(m);

  if (m == 0 ||  m == 0xFFFE)
      return "0000";

  string move = UCIsquare(from) + UCIsquare(to);
  bool isPromotion =
		  (get_rank(to) == 7 && g.current.getPiece(from) == W_PAWN) ||
		  (get_rank(to) == 0 && g.current.getPiece(from) == B_PAWN);

  if (isPromotion)
      move += "q";

  return move;
}


/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

Move UCIto_move(Game& g, string& str) {

  if (str.length() == 5) // Junior could send promotion piece in uppercase
      str[4] = char(tolower(str[4]));

  auto valid_moves = g.getValidMoves();
  for (const auto& m : valid_moves)
      if (str == UCImove(g, m, false))
          return m;

  return 0;
}



// FEN string of the initial position, normal chess
const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


  // position() is called when engine receives the "position" UCI command.
  // The function sets up the position described in the given FEN string ("fen")
  // or the starting position ("startpos") and then makes the moves given in the
  // following move list ("moves").

  void UCIposition(Game& pos, istringstream& is) {

    Move m;
    string token, fen;

    is >> token;

    if (token == "startpos")
    {
        fen = StartFEN;
        is >> token; // Consume "moves" token if any
    }
    else if (token == "fen")
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

    pos.setFEN(fen);

    // Parse move list (if any)
    while (is >> token && (m = UCIto_move(pos, token)) != 0) {
    	pos.execMove(m);
    }
  }


  // setoption() is called when engine receives the "setoption" UCI command. The
  // function updates the UCI option ("name") to the given value ("value").

  void UCIsetoption(istringstream& is) {

    string token, name, value;

    is >> token; // Consume "name" token

    // Read option name (can contain spaces)
    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    // Read option value (can contain spaces)
    while (is >> token)
        value += (value.empty() ? "" : " ") + token;
    if (name == "posscore")
    	limits.pos_score_enabled = value == "true";
    //cout << name << "=" << token << endl;
    Options[name] = value;
  }


  // go() is called when engine receives the "go" UCI command. The function sets
  // the thinking time and other parameters from the input string, then starts
  // the search.
#define WHITE 0
#define BLACK 1

  void UCIgo(Game& pos, istringstream& is) {

    string token;
    bool ponderMode = false;

    limits.startTime = now(); // As early as possible!

    while (is >> token)
        if (token == "searchmoves")
            while (is >> token)
                limits.searchmoves.push_back(UCIto_move(pos, token));
        else if (token == "wtime")     is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "posscore")  is >> limits.pos_score_enabled;
        else if (token == "mate")      is >> limits.mate_search;
        else if (token == "perft")     is >> limits.perft;
        else if (token == "infinite")  limits.infinite = 1;
        else if (token == "ponder")    ponderMode = true;
    //limits.depth += 5;
    pos.startSearchMPI(limits.depth);
    //Threads.start_thinking(pos, states, limits, ponderMode);
  }

/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.
string GetLineSync() {
    string line;
    getline(cin, line);
    return line;
}

void UCIloop(int argc, char* argv[]) {

  string token, cmd;
  for (int i = 1; i < argc; ++i)
      cmd += std::string(argv[i]) + " ";

  limits.pos_score_enabled = true;
  Game g = Game();
  limits.depth = 6;
  auto future = async(launch::async, GetLineSync);

  do {
	  bool new_result = g.processSearchQ();
	  if (evals > 0 && new_result) {
	      for (auto r : g.last_search_result)
	    	  printMoveUCI(r, g.last_search_ms+1);
		  auto move = g.selectMove(g.last_search_result);
	      if (move.move == 0)
	        	break; // mate or stale
	      // info depth 6 seldepth 4 multipv 1 score cp 59 nodes 489 nps 244500 hashfull 0 tbhits 0 time 2 pv g1f3 d7d5 d2d4
	      // bestmove g1f3 ponder d7d5
	      //cout << g.last_search_ms << endl;
	      cout << "bestmove " << g.current.move2str(move.move) << endl;
	      ResetStats();
	  }
      if (argc == 1) {
    	cmd.clear();
   		if (future.wait_for(chrono::milliseconds(5)) == future_status::ready) {
  			cmd = future.get();
  			if (cmd != "quit")
  				future = async(launch::async, GetLineSync);
   		}
      }
      if (cmd.empty())
    	  continue;
      istringstream is(cmd);

      token.clear(); // Avoid a stale if getline() returns empty or blank line
      is >> skipws >> token;

      if (    token == "quit"
          ||  token == "stop")
          g.stopSearchMPI();

      else if (token == "uci") {
          cout << "id name " << "MULLER1" << endl;
          cout << "id author " << "CH" << "\n"       //<< Options
			<< "option name Posscore type check default false\n"
			<< "uciok"  << endl;
      }
      else if (token == "setoption")  UCIsetoption(is);
      else if (token == "go")         UCIgo(g, is);
      else if (token == "position")   UCIposition(g, is);
      else if (token == "ucinewgame") g.stopSearchMPI();
      else if (token == "isready")    cout << "readyok" << endl;

      // Additional custom non-UCI commands, mainly for debugging
      //else if (token == "flip")  pos.flip();
      //else if (token == "bench") bench(pos, is, states);
      else if (token == "d") {
    	  cout << "History: " << g.board_history.size() << " MateSearch: " << limits.mate_search << " posscore: " << limits.pos_score_enabled << " depth: " << limits.depth <<endl;
    	  g.current.print(g.checkRepetition());
      }
      //else if (token == "eval")  cout << Eval::trace(pos) << endl;
      else
          cout << "Unknown command: " << cmd << endl;
  } while (token != "quit" && argc == 1); // Command line args are one-shot
}

