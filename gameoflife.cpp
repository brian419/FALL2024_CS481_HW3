/*
    Name: Jeongbin Son
    Email: json10@crimson.ua.edu
    Course Section: CS 481
    Homework #: 3
    To Compile: clang++ -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib -lomp -std=c++11 -o gameoflife gameoflife.cpp
    To Run: ./gameoflife <board size> <max generations> <num threads> <output directory>
    ex: ./gameoflife 100 100 2 /Users/brianson/Desktop/cs481/hw3/output
*/

#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <omp.h> 

using namespace std;
using namespace std::chrono;

// this function will create an output directory
void creatingoutputDirectory(const string &outputDirectory) {
    struct stat directoryInfo;

    // if the directory doesn't exist, create it
    if (stat(outputDirectory.c_str(), &directoryInfo) != 0) {
        cout << "Creating directory: " << outputDirectory << endl;
        if (mkdir(outputDirectory.c_str(), 0777) == -1) {
            cerr << "There was an error creating the directory " << outputDirectory << endl; // debug error message
            exit(1);
        }
    }
}

// this function will count the number of alive neighbors around a given cell
int countAliveNeighbors(const vector<vector<int>> &board, int x, int y) {
    int aliveNeighborsCount = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            aliveNeighborsCount += board[x + i][y + j];
        }
    }
    return aliveNeighborsCount;
}

// this function will compute the next generation of the board
void nextGeneration(vector<vector<int>> &currentGeneration, vector<vector<int>> &nextGeneration) {
    int rows = currentGeneration.size();
    int cols = currentGeneration[0].size();

    #pragma omp parallel for collapse(2) // parallelize with openmp
    for (int i = 1; i < rows - 1; ++i) {
        for (int j = 1; j < cols - 1; ++j) {
            int aliveNeighbors = countAliveNeighbors(currentGeneration, i, j);
            if (currentGeneration[i][j] == 1) {
                if (aliveNeighbors < 2 || aliveNeighbors > 3) {
                    nextGeneration[i][j] = 0; 
                }
                else {
                    nextGeneration[i][j] = 1; 
                }
            }
            else {
                if (aliveNeighbors == 3) {
                    nextGeneration[i][j] = 1; 
                }
                else {
                    nextGeneration[i][j] = 0; 
                }
            }
        }
    }
}

bool isBoardSame(const vector<vector<int>> &board1, const vector<vector<int>> &board2) {
    bool isSameBoard = true;

    #pragma omp parallel for
    for (size_t i = 1; i < board1.size() - 1; ++i) {
        bool localisSameBoard = true;  

        for (size_t j = 1; j < board1[i].size() - 1; ++j) {
            if (board1[i][j] != board2[i][j]) {
                localisSameBoard = false;  // setting local variable to false if boards are diff
            }
        }
        #pragma omp critical // critical section
        {
            if (!localisSameBoard) {
                isSameBoard = false;
            }
        }
    }
    return isSameBoard;
}

// this function will write the final board to a file
void writingFinalBoardToFile(const vector<vector<int>> &board, const string &outputDirectory, int generation) {
    creatingoutputDirectory(outputDirectory);  
    ofstream outFile(outputDirectory + "/final_board_gen_" + to_string(generation) + ".txt");
    for (size_t i = 1; i < board.size() - 1; ++i) {
        for (size_t j = 1; j < board[i].size() - 1; ++j) {
            outFile << (board[i][j] ? '*' : '.') << " ";
        }
        outFile << endl;
    }
    outFile.close();
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cout << "Incorrect usage. Please type '" << argv[0] << " <board size> <max generations> <num threads> <output directory> \nex: ./gameoflife 100 100 2 /Users/brianson/Desktop/cs481/hw3/output'" << endl;
        return 1;
    }

    // command line arguments
    string outputDirectory = argv[4];
    int boardSize = stoi(argv[1]);         
    int maxAmountGenerations = stoi(argv[2]);    
    int numThreads = stoi(argv[3]);        

    omp_set_num_threads(numThreads);       

    vector<vector<int>> board(boardSize + 2, vector<int>(boardSize + 2, 0)); 
    vector<vector<int>> nextGenerationBoard(boardSize + 2, vector<int>(boardSize + 2, 0));

    srand(12345); // fixed seed

    for (int i = 1; i <= boardSize; ++i) {
        for (int j = 1; j <= boardSize; ++j) {
            board[i][j] = rand() % 2;
        }
    }

    auto start = high_resolution_clock::now(); 

    int generation = 0;   
    bool noChange = false; 

    while (generation < maxAmountGenerations && !noChange) {
        nextGeneration(board, nextGenerationBoard); 

        if (isBoardSame(board, nextGenerationBoard)) {
            noChange = true;  
        }

        board.swap(nextGenerationBoard); 
        generation++;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Simulation completed in " << generation << " generations and took " << duration.count() << " ms." << endl;

    writingFinalBoardToFile(board, outputDirectory, generation);

    return 0;
}
