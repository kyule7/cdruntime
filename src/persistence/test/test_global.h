#pragma once

int myRankID = 0;
int totalRanks = 1;
#define TEST_PRINT(...) if(myRankID == 0) { printf(__VA_ARGS__); }
