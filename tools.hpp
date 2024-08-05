
enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK


/// xorshift64star Pseudo-Random Number Generator
/// This class is based on original code written and dedicated
/// to the public domain by Sebastiano Vigna (2014).
/// It has the following characteristics:
///
///  -  Outputs 64-bit numbers
///  -  Passes Dieharder and SmallCrush test batteries
///  -  Does not require warm-up, no zeroland to escape
///  -  Internal state is a single 64-bit integer
///  -  Period is 2^64 - 1
///  -  Speed: 1.60 ns/call (Core i7 @3.40GHz)
///
/// For further analysis see
///   <http://vigna.di.unimi.it/ftp/papers/xorshift.pdf>

class PRNG {

  uint64_t s;

  uint64_t rand64() {

    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return s * 2685821657736338717LL;
  }

public:
  PRNG(uint64_t seed) : s(seed) { assert(seed); }

  template<typename T> T rand() { return T(rand64()); }

  /// Special generator used to fast init magic numbers.
  /// Output values only have 1/8th of their bits set on average.
  template<typename T> T sparse_rand()
  { return T(rand64() & rand64() & rand64()); }
};

void sleep_us(unsigned long microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000ul;            // whole seconds
    ts.tv_nsec = (microseconds % 1000000ul) * 1000;  // remainder, in nanoseconds
    nanosleep(&ts, NULL);
}

void sleep_ms(unsigned long milliseconds) {
	sleep_us(milliseconds * 1000);
}

// simple helpers
uint16_t str2move(string move) {
    uint16_t result = 0;
    uint8_t from_row = char(move[1]) - '1';
    uint8_t from_col = char(move[0]) - 'a';
    uint8_t to_row = char(move[3]) - '1';
    uint8_t to_col = char(move[2]) - 'a';
    return move_encode(from_row * 8 + from_col, to_row, to_col);
}

void printU128(__uint128_t u) {
    bitset<128> hi{static_cast<uint64_t>(u >> 64)},
                lo{static_cast<uint64_t>(u)},
                result{(hi << 64) | lo};
    cout << result << endl;
}


void printMove(uint16_t m) {
    auto source = move_from(m);
    auto target = move_to(m);
    cout << char('a'+get_file(source)) << (get_rank(source) + 1) << char('a'+get_file(target)) << (get_rank(target) + 1) << " ";
}

void printMove(EvalResult m) {
	printMove(m.move);
	cout << "/" << m.score << " [LOT: ";
	int d = 15;
	while(m.lot[d] == 0 && d > 0) d--;
    for (int j = d-1; j > 0; j--) {
    	if (m.lot[j] == 0) {
    		cout << "MATE";break;
    	}
    	if (m.lot[j] == 0xFFFF) {
    		cout << "STALE";break;
    	}
    	if (m.lot[j] == 0xFFEE) {
    		cout << "ERR";break;
    	}
    	printMove(m.lot[j]);
    }
    cout << "] ";
}

void printMoveUCI(EvalResult m, int time_spent_ms) {
    // info depth 6 seldepth 4 multipv 1 score cp 59 nodes 489 nps 244500 hashfull 0 tbhits 0 time 2 pv g1f3 d7d5 d2d4

	cout << "info depth " << limits.depth << " score cp " << m.score << " nodes " << evals << " nps " << (evals / time_spent_ms) * 1000 << " time " << time_spent_ms << " pv ";
	int d = MAX_DEPTH - 1;
	while(m.lot[d] == 0 && d > 0) d--;
	printMove(m.move);
    for (int j = d-1; j > 0; j--) {
    	if (m.lot[j] == 0) {
    		cout << "MATE";break;
    	}
    	if (m.lot[j] == 0xFFFF) {
    		cout << "STALE";break;
    	}
    	if (m.lot[j] == 0xFFEE) {
    		cout << "ERR";break;
    	}
    	printMove(m.lot[j]);
    }
    cout << "\n";
}

E_PIECE charToPiece(unsigned char c) {
	const string p2c(" PKQRBN rbnkqp  ");
	E_PIECE result = P_EMPTY;
	auto idx = p2c.find(c);
	if (idx != string::npos)
		result = static_cast<E_PIECE>(idx);
	return result;
}

string Board::move2str(Move m) {
    auto source = move_from(m);
    auto target = move_to(m);
    auto pc = getPiece(source);
    string promotion;
    if (pc == W_PAWN && get_rank(target) == 7)
    	promotion = "q";
    if (pc == B_PAWN && get_rank(target) == 0)
    	promotion = "q";

    stringstream ss;
    ss << char('a'+get_file(source)) << (get_rank(source) + 1) << char('a'+get_file(target)) << (get_rank(target) + 1) << promotion << " ";
    return ss.str();
}

void Board::print(int rep) {
    for (int row = 7; row >= 0; row--) {
        cout << row+1 << " ";
        for (int col = 0; col < 8; col++) {
            uint8_t pos = row * 8 + col;
            if (has_bit(position, pos)) {
                auto pc_idx = get_pcidx(position, pos);
                auto pc = get_pc(pc_idx, pieces_single);
                switch (pc) {
                case W_PAWN: cout << "♙";break;
                case W_ROOK: cout << "♖";break;
                case W_BISHOP: cout << "♗";break;
                case W_KNIGHT: cout << "♘";break;
                case W_QUEEN: cout << "♕";break;
                case W_KING: cout << "♔";break;
                case B_PAWN: cout << "♟︎";break;
                case B_ROOK: cout << "♜";break;
                case B_BISHOP: cout << "♝";break;
                case B_KNIGHT: cout << "♞";break;
                case B_QUEEN: cout << "♛";break;
                case B_KING: cout << "♚";break;
                default: cout << "?";break;
                }
            } else 
                cout << ".";
            cout << " ";
        }
        cout << "\n";
    }
   	cout << "  ";
    for (int col = 0; col < 8; col++)
    	cout << char('A'+col) << " "; 
    cout << "\n";
    MoveArray tmp;
    uint16_t n_moves_white = moves(true, tmp);removeInvalid(true, n_moves_white, tmp);
    uint16_t n_moves_black = moves(false, tmp);removeInvalid(false, n_moves_black, tmp);
    cout << "Eval: " << eval() << " Check W: " << isCheck(true) << " Check B: " << isCheck(false);
    if (rep >= 0)
    	cout << " Repetition: " << rep;
    cout << " Possible moves white: " << n_moves_white << " Possible moves black: " << n_moves_black << " Enpassant: " << int(enpassant_square) << " Castling: " << game_flags << endl;
    evals--;
    /*
    auto m = moves(true,tmp);
    for (int i = 0; i < m; i++) {
    	cout << i<<":";printMove(tmp[i]);cout << endl;
    }*/
}
