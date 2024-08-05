
/* From Stockfish: Time goodies and limits */

typedef std::chrono::milliseconds::rep TimePoint; // A value in milliseconds
static_assert(sizeof(TimePoint) == sizeof(int64_t), "TimePoint should be 64 bits");


TimePoint now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
}
TimePoint since(TimePoint then) {
//    return duration<double,milli>(then - now()).count();
    return (now() - then);
}

/// LimitsType struct stores information sent by GUI about available time to
/// search the current move, maximum depth/time, or if we are in analysis mode.
struct LimitsType {
  bool use_time_management() const {
    return !(mate_search | movetime | depth | nodes | perft | infinite);
  }
  std::vector<Move> searchmoves;
  TimePoint time[2], inc[2], npmsec, movetime, startTime;
  int movestogo, depth, mate_search, perft, infinite;
  uint64_t nodes;
  bool pos_score_enabled, debug_mainline;
} limits = {};


// piece numbers and values. 

enum E_PIECE {
    P_EMPTY         = 0,
    W_PAWN          = 1,  // 0001
    W_KING          = 2,  // 0010
    W_QUEEN         = 3,  // 0100
    W_ROOK          = 4,  // 0101 
    W_BISHOP        = 5,  // 0110
    W_KNIGHT        = 6,  // 0111
    B_ROOK          = 8,  // 1001
    B_BISHOP        = 9,  // 1010
    B_KNIGHT        = 10, // 1011
    B_KING          = 11, // 1100
    B_QUEEN         = 12, // 1110
    B_PAWN          = 13, // 1000
};

enum E_GFLAGS_BIT {
    W_CK_BIT = 0,  // White can castle king-side
    W_CQ_BIT = 1,  // White can castle queen-side
    B_CK_BIT = 2,  // Black can castle king-side
    B_CQ_BIT = 3,  // Black can castle queen-side
};

enum E_GFLAGS_VAL {
    W_CK_VAL = 1,  // White can castle king-side
    W_CQ_VAL = 2,  // White can castle queen-side
    B_CK_VAL = 4,  // Black can castle king-side
    B_CQ_VAL = 8,  // Black can castle queen-side
};

//                         0   1       2         3    4    5    6    7     8     9    10     11           12     13       14
const int32_t value[16] = {0, 100, INT32_MAX/2, 900, 500, 300, 300,  0,  -500, -300, -300, -INT32_MAX/2, -900, -100, 0, 0};
//const int32_t value[16] = {0, 100, INT32_MAX/2, INT32_MAX/2, 900, 500, 300, 300, 100, 500, 300, 300, INT32_MAX/2, INT32_MAX/2, 900};

// bit op helpers
//static inline bool has_bit(uint64_t &x, int bitNum) { return x & (1ULL << bitNum); }
static inline bool has_bit(uint64_t x, int bitNum) { return x & (1ULL << bitNum); }
static inline bool has_bit(uint64_t &x, int row, int col) { return x & (1ULL << (row * 8 + col)); }
static inline void set_bit(uint64_t &x, int bitNum) { x |= (1ULL << bitNum); }
static inline void clear_bit(uint64_t &x, int bitNum) { x &= ~(1ULL << (bitNum)); }
static inline void set_bit32(uint32_t &x, int bitNum) { x |= (1UL << bitNum); }
static inline void clear_bit32(uint32_t &x, int bitNum) { x &= ~(1UL << (bitNum)); }
static inline uint8_t get_rank(uint32_t bitNum) { return bitNum >> 3;}
static inline uint8_t get_file(uint32_t bitNum) { return bitNum & 0b111;}
static inline uint8_t get_bitpos(uint8_t row, uint8_t col) { return (row << 3) + col;}
static inline uint8_t get_pcidx(uint64_t position, uint32_t bitpos) {return popcount((((1ULL << (bitpos))-1)) & position);}
static inline E_PIECE get_pc(uint8_t pc_idx, __uint128_t pieces) {return static_cast<E_PIECE>((pieces >> (4 * pc_idx)) & 0xF);}
static inline uint8_t move_from(uint16_t _m) { return _m & 0x3F;}
static inline uint8_t move_to(uint16_t _m) { return _m >> 6;}
static inline uint16_t move_encode(const uint8_t from, const uint8_t to_row, const uint8_t to_col) { return ((uint16_t)(to_row * 8 + to_col) << 6) | from;}
static inline uint16_t move_encode(uint8_t from, uint8_t to) { return ((uint16_t)to << 6) | from;}

const __uint128_t u128_one = 1;
const __uint128_t u128_4one = 0b1111;

// Global stats
uint64_t evals = 0;
uint64_t ab_cuts = 0;
uint64_t checks = 0;
uint64_t stales = 0;
void ResetStats() {evals = ab_cuts = checks = stales = 0;};

// castling moves
uint16_t str2move(string move); // fwd decl
const uint16_t w_o_o = str2move("e1g1");
const uint16_t wr_o_o = str2move("h1f1");
const uint16_t w_o_o_o = str2move("e1c1");
const uint16_t wr_o_o_o = str2move("a1d1");
const uint16_t b_o_o = str2move("e8g8");
const uint16_t br_o_o = str2move("h8f8");
const uint16_t b_o_o_o = str2move("e8c8");
const uint16_t br_o_o_o = str2move("a8d8");
const uint64_t w_o_o_mask = 0b11110000; // we only care for pieces between king and rook
const uint64_t w_o_o_ok =   0b10010000; // only rook and king should be present after mask
const uint64_t w_o_o_o_mask = 0b00011111; // we only care for pieces between king and rook
const uint64_t w_o_o_o_ok =   0b00010001; // only rook and king should be present after mask
const uint64_t b_o_o_mask = 0b11110000ULL << (7 * 8); // we only care for pieces between king and rook
const uint64_t b_o_o_ok =   0b10010000ULL << (7 * 8); // only rook and king should be present after mask
const uint64_t b_o_o_o_mask = 0b00011111ULL << (7 * 8); // we only care for pieces between king and rook
const uint64_t b_o_o_o_ok =   0b00010001ULL << (7 * 8); // only rook and king should be present after mask

// Fix max of 128 moves in a movelist
typedef Move MoveArray[128];

#define MAX_DEPTH 16
typedef Move LOTArray[MAX_DEPTH];  // LineOfThought array returned from search, should be minimal to avoid copies

struct EvalResult {
    int score;
    uint16_t move;
    uint16_t depth; // the originating depth of that result (0 == full depth)
    LOTArray lot; // line-of-thought (best moves, index is depth)
};
bool operator< (const EvalResult& c1, const EvalResult& c2) { return c1.score < c2.score; };
bool operator> (const EvalResult& c1, const EvalResult& c2) { return operator<(c2, c1); };

// Returns all possible moves with a score, highest first. Empty means either mate or stale
typedef vector<EvalResult> ExtendedEvalResult;


// Describes all positions of a board. Should be minimal in data size to utilize cache
// 128+64 bit, or 24byte
struct Board {

    // ---------------- Data -----------------
    union {
        __uint128_t pieces_single;   // Max of 32 pcs, each 4 bit = 128bit
        uint64_t pieces[2];
    };
    uint64_t position;    // Piece positions as 8x8 bit map, 0 = no piece, 1 = piece. Which one decides the order. Max of 32 bits should be set.
    uint32_t enpassant_square; // 65 if no square is enpassant
    uint32_t game_flags;

    // ------------------ Methods ---------------
    int32_t eval() { // Always from white perspective
        int32_t result = 0;
        evals++;
        if (limits.mate_search)
            return 0; // draw pos

        auto pcs1 = pieces[0];
        auto pcs2 = pieces[1];
        /* variant 1 faster with many parts
        result += value[(pcs1 >> 0 ) & 0xF] + value[(pcs2 >> 0 ) & 0xF];
        result += value[(pcs1 >> 4 ) & 0xF] + value[(pcs2 >> 4 ) & 0xF];
        result += value[(pcs1 >> 8 ) & 0xF] + value[(pcs2 >> 8 ) & 0xF];
        result += value[(pcs1 >> 12) & 0xF] + value[(pcs2 >> 12) & 0xF];
        result += value[(pcs1 >> 16) & 0xF] + value[(pcs2 >> 16) & 0xF];
        result += value[(pcs1 >> 20) & 0xF] + value[(pcs2 >> 20) & 0xF];
        result += value[(pcs1 >> 24) & 0xF] + value[(pcs2 >> 24) & 0xF];
        result += value[(pcs1 >> 28) & 0xF] + value[(pcs2 >> 28) & 0xF];
        result += value[(pcs1 >> 32) & 0xF] + value[(pcs2 >> 32) & 0xF];
        result += value[(pcs1 >> 36) & 0xF] + value[(pcs2 >> 36) & 0xF];
        result += value[(pcs1 >> 40) & 0xF] + value[(pcs2 >> 40) & 0xF];
        result += value[(pcs1 >> 44) & 0xF] + value[(pcs2 >> 44) & 0xF];
        result += value[(pcs1 >> 48) & 0xF] + value[(pcs2 >> 48) & 0xF];
        result += value[(pcs1 >> 52) & 0xF] + value[(pcs2 >> 52) & 0xF];
        result += value[(pcs1 >> 56) & 0xF] + value[(pcs2 >> 56) & 0xF];
        result += value[(pcs1 >> 60) & 0xF] + value[(pcs2 >> 60) & 0xF];
		/* variant 2 faster with fewer parts?*/
        /*
        int32_t c = popcount(position);
        int32_t c1 = min(c, 16);
        int32_t c2 = c <= 16 ? 0 : c - 16;
        while (c1--) {
        	result += value[pcs1 & 0xF];
        	pcs1 >>= 4;
        }
        while (c2--) {
        	result += value[pcs2 & 0xF];
        	pcs2 >>= 4;
        }
        */
         /* variant 3 */
        uint64_t set_a = pieces[0];
        uint64_t set_b = pieces[1];
        for (int i = 0; i < 16; i++) {
            result += value[set_a & 0xF] + value[set_b & 0xF];
            set_a >>= 4;
            set_b >>= 4;
        }
        /* variant 4 with complete pos iteration 
        uint32_t pc_idx = 0;
        for (uint32_t current_bit = countr_zero(position); current_bit < 64; current_bit+= countr_zero(position >> (current_bit+1)) + 1) { // Loop over all non zero bitpos
            const auto pc = get_pc(pc_idx, pieces_single);
            pc_idx++;
            result += value[pc];
        }
    */
        //uint16_t dummy[128];
        //result += moves(true, dummy); // score based on available moves, unfortunately very expensive. Better: lazy approach one depth back via recursion
        //result -= moves(false, dummy);*/
        return result;
    }

    // Returns piece at bitpos
    E_PIECE getPiece(uint32_t bitpos) {
    	if (!has_bit(position, bitpos))
    		return P_EMPTY;
    	auto pcidx = get_pcidx(position, bitpos);
    	return get_pc(pcidx, pieces_single);
    }

    void remove(uint32_t bitpos) {
        if (!has_bit(position, bitpos))
            return;
        uint32_t pc_idx = get_pcidx(position, bitpos);
        removeFast(bitpos, pc_idx);
    }

    void removeFast(uint32_t bitpos, uint32_t pc_idx) {
        const auto oshift = pieces_single >> 4;
        const __uint128_t mask = (u128_one << (4 * pc_idx)) - 1;
        pieces_single = (pieces_single & mask) | (oshift & ~mask);
        clear_bit(position, bitpos);
    }
    
    E_PIECE insert(uint8_t pc, uint32_t bitpos) {  // returns: was take?
        bool takes = has_bit(position, bitpos);
        E_PIECE result = P_EMPTY;
        uint32_t pc_idx = get_pcidx(position, bitpos);
        if (!takes) {  // insert at position
            const auto oshift = pieces_single << 4;
            const __uint128_t mask = (u128_one << (4 * pc_idx)) - 1;
            pieces_single = (pieces_single & mask) | (oshift & ~mask);
        } else
            result = get_pc(pc_idx, pieces_single);
        pieces_single &= ~(u128_4one << (4 * pc_idx)); // Zero part for new pc
        pieces_single |= static_cast<__uint128_t>(pc) << (4 * pc_idx); //insert
        set_bit(position, bitpos);
        return result;
    }

    E_PIECE insert(uint8_t pc, uint8_t row, uint8_t col) {  // returns: was take?
        return insert(pc, (8 * row + col));
    }

    void replaceAll(E_PIECE pc, E_PIECE pc_with, E_PIECE pc2 = P_EMPTY, E_PIECE pc2_with = P_EMPTY) {  // Replace piece pc with pc_with, not touching position
        __uint128_t new_list = 0;
        int32_t pc_count = popcount(position);
        while (--pc_count > 0) {
            auto pc_current = get_pc(pc_count, pieces_single);
            if (pc_current == pc)
                pc_current = pc_with;
            else if (pc_current == pc2)
                pc_current = pc2_with;
            new_list <<= 4;
            new_list |= pc_current;
        }
        pieces_single = new_list;
    }

    Board move(uint16_t m, E_PIECE &taken) { // moves a piece from a to b, not checking for legality
        return move(m, get_pcidx(position, move_from(m)), taken);
    };

    Board move(uint16_t m, uint32_t pc_idx, E_PIECE &taken) { // moves a piece from a to b, not checking for legality
        Board board = *this;  // may be slow because memcpy gets invoked
        board.enpassant_square = 65;
        E_PIECE pc = get_pc(pc_idx, pieces_single);
        const auto move_src = move_from(m);
        const auto move_target = move_to(m);
        board.removeFast(move_src, pc_idx);
        if (pc == W_PAWN && get_rank(move_target) == 7) pc = W_QUEEN;
        if (pc == B_PAWN && get_rank(move_target) == 0) pc = B_QUEEN;        
        if (pc == W_PAWN && get_rank(move_target) - get_rank(move_src) == 2) board.enpassant_square = move_target - 8;
        if (pc == B_PAWN && get_rank(move_src) - get_rank(move_target) == 2) board.enpassant_square = move_target + 8;
        if (pc == W_KING) {
            if (m == w_o_o) 
                board = board.move(wr_o_o, taken);
            else if (m == w_o_o_o) 
                board = board.move(wr_o_o_o, taken);
            clear_bit32(board.game_flags, W_CK_BIT);
            clear_bit32(board.game_flags, W_CQ_BIT);
        }
        if (pc == B_KING) {
            if (m == b_o_o) 
                board = board.move(br_o_o, taken);
            else if (m == b_o_o_o) 
                board = board.move(br_o_o_o, taken);
            clear_bit32(board.game_flags, B_CK_BIT);
            clear_bit32(board.game_flags, B_CQ_BIT);
        }
        // A rook move, clear eventual castling rights
        if (board.game_flags > 0) { // castling right left?
            if (pc == W_ROOK && move_src == 0 && (board.game_flags & W_CQ_VAL))
                clear_bit32(board.game_flags, W_CQ_BIT);
            if ((pc == W_ROOK) && (move_src == 7) && (board.game_flags & W_CK_VAL))
                clear_bit32(board.game_flags, W_CK_BIT);
            if (pc == B_ROOK && move_src == 63-7 && (board.game_flags & B_CQ_VAL))
                clear_bit32(board.game_flags, B_CQ_BIT);
            if (pc == B_ROOK && move_src == 63   && (board.game_flags & B_CK_VAL))
                clear_bit32(board.game_flags, B_CK_BIT);
        }
        taken = board.insert(pc, move_to(m));
        if (move_target == enpassant_square) { // enpassant take
        	if (pc == W_PAWN) {
            	board.remove(move_target-8);
            	taken = B_PAWN;
        	} else if (pc == B_PAWN) {
            	board.remove(move_target+8);
            	taken = W_PAWN;
        	}
        }
        return board;
    }

    bool isCheck(bool white) {
        MoveArray moves_opposite;
        E_PIECE own_king = white ? W_KING : B_KING;
        uint16_t n_moves = moves(!white, moves_opposite);
        E_PIECE taken;
        for (int i = 0; i < n_moves; i++) {
            move(moves_opposite[i], taken);
            if (taken == own_king)
            	return true;
        }
        return false;
    }

    // Removes invalid moves by forward iteration. Provide moves list from moves(). white determines if the provided list are moves from white or black
    // Modifies the moves list. Expensive op.
    void removeInvalid(bool white, uint16_t &n_moves, MoveArray moves, int depth = 0) {
        MoveArray new_moves;
        uint16_t n_new_moves = 0;
        E_PIECE own_king = white ? W_KING : B_KING;

        for (int i = 0; i < n_moves; i++) {
            E_PIECE taken;
            Board c = move(moves[i], taken);
            MoveArray c_moves;
            uint16_t n_c_moves = c.moves(!white, c_moves);

            if (depth > 0)
                c.removeInvalid(!white, n_c_moves, c_moves, depth-1);
            for (int j = 0; j < n_c_moves; j++) {
                Board d = c.move(c_moves[j], taken);
                if (taken == own_king) {
                    moves[i] = 0;
                    break;
                }
            }
        }
        for (int i = 0; i < n_moves; i++) 
            if (moves[i] != 0)
                new_moves[n_new_moves++] = moves[i];
        n_moves = n_new_moves;
        for (int i = 0; i < n_moves; i++) 
            moves[i] = new_moves[i];        
    }

#define ADD_MOVE(__row, __col) has_piece = has_bit(position, __row, __col); has_opponent = has_bit(opponent, __row, __col); if ((__row) < 8 && (__col) < 8 && (__row) >=0 && (__col) >= 0)  if (!has_piece || has_opponent) moves_[m++] = move_encode(current_bit, __row, __col);
    uint32_t moves(bool white, MoveArray moves_, uint64_t *opponent_ = nullptr) { // Create boards with all possible moves for white or black. Returns the number of moves stored. pointers on moves_ and boards must hold enough space.
        uint8_t m = 0;
        uint8_t pc_idx = 0;
        uint32_t opponent_king_pos = 0;
        uint64_t opponent = 0;
        uint64_t enpassant = enpassant_square > 63 ? 0 : 1ULL << enpassant_square;
        for (uint32_t current_bit = countr_zero(position); current_bit < 64; current_bit+= countr_zero(position >> (current_bit+1)) + 1) { // Loop over all non zero bitpos
            const auto pc = get_pc(pc_idx, pieces_single);
            pc_idx++;
            if (white && pc == B_KING) opponent_king_pos = current_bit;
            else if (!white && pc == W_KING) opponent_king_pos = current_bit;
            if ((white && pc > 7) || (!white && pc < 8)) // mark white pieces only
                set_bit(opponent, current_bit);
        }
        if (opponent_ != nullptr)
            *opponent_ = opponent;
        //auto add_move = [&]()

        pc_idx = 0;
        for (uint32_t current_bit = countr_zero(position); current_bit < 64; current_bit+= countr_zero(position >> (current_bit+1)) + 1) { // Loop over all non zero bitpos
            const auto pc = get_pc(pc_idx, pieces_single);
            pc_idx++;
            if (white && pc > 7) continue; // we should not move black pieces if white
            if (!white && pc <= 7) continue; // we should not move white pieces if black
            //cout << current_bit << ": " << (int)pc << endl;

            const uint8_t row = get_rank(current_bit);
            const uint8_t col = get_file(current_bit);
            bool has_piece, has_opponent;
            if (pc == P_EMPTY)
                continue;
            if (pc == W_PAWN) {
                if (!has_bit(position, current_bit + 8)) {
                    moves_[m++] = move_encode(current_bit, row+1, col);
                    if (row == 1 && !has_bit(position, current_bit + 16)) 
                        moves_[m++] = move_encode(current_bit, row+2, col);
                }
                if (col > 0 && row < 7 && has_bit(opponent | enpassant, current_bit + 7))
                    moves_[m++] = move_encode(current_bit, row+1, col-1);
                if (col < 7 && row < 7 && has_bit(opponent | enpassant, current_bit + 9))
                    moves_[m++] = move_encode(current_bit, row+1, col+1);
                continue; // NEXT PIECE
            }
            if (pc >= B_PAWN) {
                if (!has_bit(position, current_bit - 8)) {
                    moves_[m++] = move_encode(current_bit, row-1, col);
                    if (row == 6 && !has_bit(position, current_bit - 16)) 
                        moves_[m++] = move_encode(current_bit, row-2, col);
                }
                if (col > 0 && row > 0 && has_bit(opponent | enpassant, current_bit - 9))
                    moves_[m++] = move_encode(current_bit, row-1, col-1);
                if (col < 7 && row > 0 && has_bit(opponent | enpassant, current_bit - 7))
                    moves_[m++] = move_encode(current_bit, row-1, col+1);
                continue; // NEXT PIECE
            }
            if (pc == W_BISHOP || pc == B_BISHOP || pc == W_QUEEN || pc == B_QUEEN) {
                for (uint8_t l = 1; l <= min<uint8_t>(7-row, 7-col); l++) {  // right upper
                    ADD_MOVE(row+l, col+l)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l <= min<uint8_t>(7-row, col); l++) {  // left upper
                    ADD_MOVE(row+l, col-l)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l <= min(row, col); l++) {  // left lower
                    ADD_MOVE(row-l, col-l)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l <= min<uint8_t>(row, 7-col); l++) {  // right lower
                    ADD_MOVE(row-l, col+l)
                    if (has_piece || has_opponent)
                        break;
                }
            }
            if (pc == W_ROOK || pc == B_ROOK || pc == W_QUEEN || pc == B_QUEEN) {
                for (uint8_t l = 1; l < 8-col; l++) {  // right 
                    ADD_MOVE(row, col+l)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l <= col; l++) {  // left 
                    ADD_MOVE(row, col-l)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l < 8 - row; l++) {  // up
                    ADD_MOVE(row+l, col)
                    if (has_piece || has_opponent)
                        break;
                }
                for (uint8_t l = 1; l <= row; l++) {  // down
                    ADD_MOVE(row-l, col)
                    if (has_piece || has_opponent)
                        break;
                }
            }
            if (pc == W_KNIGHT || pc == B_KNIGHT) {
                ADD_MOVE(row-2, col-1)
                ADD_MOVE(row-2, col+1)
                ADD_MOVE(row+2, col-1)
                ADD_MOVE(row+2, col+1)
                ADD_MOVE(row-1, col-2)
                ADD_MOVE(row+1, col-2)
                ADD_MOVE(row-1, col+2)
                ADD_MOVE(row+1, col+2)
                continue;
            }
            if (pc == W_KING || pc == B_KING) {
                ADD_MOVE(row, col+1)
                ADD_MOVE(row, col-1)
                ADD_MOVE(row-1, col)
                ADD_MOVE(row+1, col)
                ADD_MOVE(row-1, col+1)
                ADD_MOVE(row+1, col+1)
                ADD_MOVE(row-1, col-1)
                ADD_MOVE(row+1, col-1)
                if (pc == W_KING && ((position & w_o_o_mask) == w_o_o_ok) && (game_flags & W_CK_VAL)) { // O-O
                    ADD_MOVE(row, col+2)
                }
                if (pc == W_KING && ((position & w_o_o_o_mask) == w_o_o_o_ok) && (game_flags & W_CQ_VAL)) {// O-O-O
                    ADD_MOVE(row, col-2)
                }
                if (pc == B_KING && ((position & b_o_o_mask) == b_o_o_ok) && (game_flags & B_CK_VAL)) { // O-O
                    ADD_MOVE(row, col+2)
                }
                if (pc == B_KING && ((position & b_o_o_o_mask) == b_o_o_o_ok) && (game_flags & B_CQ_VAL)) {// O-O-O
                    ADD_MOVE(row, col-2)
                }
            }           
        }
        #ifdef ENG_ORDER_MOVES
        if (m <= 2) 
            return m;
        // Prioritize moves that capture
        
        uint32_t front = 0;
        uint32_t back = m-1;
        
        while (front < back) {
            while (has_bit(opponent, move_to(moves_[front])) && front < back)
                front++;
            while (!has_bit(opponent, move_to(moves_[back])) && front < back)
                back--;
            if (front < back)
                swap(moves_[front++], moves_[back--]);
        }
        // if opponent king is under take-moves, put first!
        for (uint32_t i = 1; i < front; i++) 
            if (move_to(moves_[i]) == opponent_king_pos) {
                swap(moves_[0], moves_[i]);
                break;
            }
        
        // Order by value captured, C++ way
    /*
        array<pair<uint16_t,int>, 128> move_score;
        int wmult = white ? -1 : 1;
        for (int i = 0; i < m; i++) {
            auto target_sq = move_to(moves_[i]);
            move_score[i] = make_pair(moves_[i], has_bit(opponent, target_sq) ? wmult * value[getPiece(target_sq)] : 0);
            //cout << i << ":"<< move_score[i] << " " << moves_[i] << endl;
        }
        stable_sort(move_score.begin(), move_score.begin()+m, [&](pair<uint16_t,int> i, pair<uint16_t,int> j){return i.second > j.second;} );
        for (int i = 0; i < m; i++) 
            moves_[i] = move_score[i].first;
        //    cout << "x"<< i << ":"<< move_score[i].second << " " << move_score[i].first << endl;
      */  
        /*
        union MoveBuffer {
            uint16_t moves_16[128];
            uint64_t moves_64[128/4];
        };
        MoveBuffer backup;
        MoveBuffer *source = (MoveBuffer *)moves_;
        for (uint8_t i = 0; i < (m >> 2)+1; i++)
            backup.moves_64[i] = (*source).moves_64[i];
        uint8_t front = 0;
        uint8_t back = m-1;
        for (uint8_t i = 0; i < m; i++) {
            if (has_bit(opponent, move_to(backup.moves_16[i]))) {
                moves_[front++] = backup.moves_16[i];
               	continue;
            }
            moves_[back--] = backup.moves_16[i];
        }
        */
        #endif
        return m;
    }
#undef ADD_MOVE
    
    void print(int rep = -1);
    string move2str(Move m);
    void clear() {
        pieces_single = position = game_flags = 0; enpassant_square = 65;
    }
};
bool operator== (const Board& c1, const Board& c2) { return (c1.position == c2.position) && (c1.pieces_single == c2.pieces_single); };

volatile int *engine_halt; // Goes to MPI window 0 that signals stop / timeout

EvalResult f_negamax(int depth, Board &x, int alpha, int beta, bool white, int16_t white_mc, int16_t black_mc) {
    EvalResult result{};
    if (depth == 0) {
        result.score = x.eval();
        if (limits.pos_score_enabled)
            result.score += white_mc - black_mc;  // Enabling drastically reduces possible branch cuts
        if (!white)
            result.score = -result.score;
        return result;
    }
    uint64_t opponent;
    MoveArray moves;
    uint16_t m = x.moves(white, moves, &opponent);
    E_PIECE taken = P_EMPTY;
    result.score = INT32_MIN + 1;
    result.move = 0;
    // Count moves that threaten an opponent
    uint16_t threats = 0;
    if (limits.pos_score_enabled)
        for (int i = 0; i < m; i ++) 
            if (has_bit(opponent, move_to(moves[i])))
                threats++;
         
    for (int i = 0; i < m; i ++) {
        Board new_board = x.move(moves[i], taken);

        if (taken == W_KING || taken == B_KING) {
            result.score = -value[taken] - (white ? -depth : depth); // include depth as score to prolong mate as long as possible. otherwise all mates (fast or slow) look equal in score!
            if (!white)
                result.score = -result.score;
            result.lot[depth] = result.move = moves[i];
            result.depth = depth;
            for (int j = 0; j < depth; j++)
                result.lot[j] = 0;
            checks++;
            evals++;
            return result;
        }

        EvalResult eval_pos = f_negamax(depth - 1, new_board, -beta, -alpha, !white, white ? threats : white_mc, white ? black_mc : threats);
        if (-eval_pos.score > result.score) {
            result.score = -eval_pos.score;
            result.depth = eval_pos.depth;
            result.lot[depth] = result.move = moves[i];
            for (int j = 0; j < depth; j++)
                result.lot[j] = eval_pos.lot[j];
            #ifdef ENG_AB_CUT
            alpha = max(alpha, result.score);
            if (alpha >= (beta - ENG_POS_SCORE_ACCURACY)) {
                ab_cuts++;
                return result; // (* cut-off *)
            }
            #endif
        }
        if (*engine_halt)
        	return result;
    }

    // if all possible moves lead to loss of king, check the null move to rule out stale mate.
    if ((result.score <= value[B_KING]) && (depth - result.depth == 1)) { // only check if next move would be fatal
        EvalResult eval_pos = f_negamax(1, x, INT32_MIN + 1, INT32_MAX, !white, 0, 0);
    	x.move(eval_pos.move, taken); // execute best move and see which piece is taken (if any)
    	if ((white && taken != W_KING) || (!white && taken != B_KING)) { // best move is not to take the king, stalemate!
            stales++;
            result.score = 0; //draw
            result.move = 0;
            result.depth = depth;
            for (int j = 0; j < depth; j++)
                result.lot[j] = 0;
        }
    }
    return result;
}

// https://en.wikipedia.org/wiki/Principal_variation_search
EvalResult f_pvs(int depth, Board &x, int alpha, int beta, bool white, int16_t white_mc, int16_t black_mc) {
    EvalResult result{};
    if (depth == 0) {
        result.score = x.eval();
        if (limits.pos_score_enabled)
            result.score += white_mc - black_mc;  // Enabling drastically reduces possible branch cuts
        if (!white)
            result.score = -result.score;
        return result;
    }
    uint64_t opponent;
    MoveArray moves;
    uint16_t m = x.moves(white, moves, &opponent);
    E_PIECE taken = P_EMPTY;
    result.score = INT32_MIN + 1;
    result.move = 0;
    // Count moves that threaten an opponent
    int16_t &threats = white ? white_mc : black_mc;
    if (limits.pos_score_enabled)
        for (int i = 0; i < m; i ++) 
            if (has_bit(opponent, move_to(moves[i])))
                threats++;
         
    for (int i = 0; i < m; i ++) {
        Board new_board = x.move(moves[i], taken);

        if (taken == W_KING || taken == B_KING) {
            result.score = -value[taken] - (white ? -depth : depth); // include depth as score to prolong mate as long as possible. otherwise all mates (fast or slow) look equal in score!
            if (!white)
                result.score = -result.score;
            result.lot[depth] = result.move = moves[i];
            result.depth = depth;
            for (int j = 0; j < depth; j++)
                result.lot[j] = 0;
            checks++;
            evals++;
            return result;
        }
        EvalResult eval_pos;
        if (i == 0) {
            eval_pos = f_pvs(depth - 1, new_board, -beta, -alpha, !white, white_mc, black_mc);
        } else {
            eval_pos = f_pvs(depth - 1, new_board, -alpha - 1, -alpha, !white, white_mc, black_mc);
            int score = -eval_pos.score;
            if (alpha < score && score < beta)
                eval_pos = f_pvs(depth - 1, new_board, -beta, -alpha, !white, white_mc, black_mc);
        }
        if (-eval_pos.score > result.score) {
            result.score = -eval_pos.score;
            result.depth = eval_pos.depth;
            result.lot[depth] = result.move = moves[i];
            for (int j = 0; j < depth; j++)
                result.lot[j] = eval_pos.lot[j];
            #ifdef ENG_AB_CUT
            alpha = max(alpha, result.score);
            if (alpha >= (beta - ENG_POS_SCORE_ACCURACY)) {
                ab_cuts++;
                return result; // (* cut-off *)
            }
            #endif
        }
        if (*engine_halt)
            return result;
    }

    // if all possible moves lead to loss of king, check the null move to rule out stale mate.
    if ((result.score <= value[B_KING]) && (depth - result.depth == 1)) { // only check if next move would be fatal
        EvalResult eval_pos = f_negamax(1, x, INT32_MIN + 1, INT32_MAX, !white, 0, 0);
        x.move(eval_pos.move, taken); // execute best move and see which piece is taken (if any)
        if ((white && taken != W_KING) || (!white && taken != B_KING)) { // best move is not to take the king, stalemate!
            stales++;
            result.score = 0; //draw
            result.move = 0;
            result.depth = depth;
            for (int j = 0; j < depth; j++)
                result.lot[j] = 0;
        }
    }
    return result;
}
/*
function pvs(node, depth, α, β, color) is
    if depth = 0 or node is a terminal node then
        return color × the heuristic value of node
    for each child of node do
        if child is first child then
            score := −pvs(child, depth − 1, −β, −α, −color)
        else
            score := −pvs(child, depth − 1, −α − 1, −α, −color) (* search with a null window *)
            if α < score < β then
                score := −pvs(child, depth − 1, −β, −α, −color) (* if it failed high, do a full re-search *)
        α := max(α, score) 
        if α ≥ β then
            break (* beta cut-off *)
    return α
    */
