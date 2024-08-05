muller: muller.cpp tools.hpp engine.hpp game.hpp uci.hpp
	mpicxx --std=c++20 -march=native -W -O5 -o muller muller.cpp
# 