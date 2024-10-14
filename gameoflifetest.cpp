#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h> // For checking/creating directories
#include <sys/types.h> // For permissions
#include <omp.h>  // Include OpenMP
//clang++ -Xpreprocessor -fopenmp -I/usr/local/opt/libomp/include -L/usr/local/opt/libomp/lib -lomp -std=c++11 -o gameoflifetest gameoflifetest.cpp

using namespace std;
using namespace std::chrono;

// Function to create the output directory if it doesn't exist
void createOutputDirectory(const string &outputDir)
{
    struct stat info;

    // Check if the directory exists
    if (stat(outputDir.c_str(), &info) != 0)
    {
        // Directory doesn't exist, so create it
        cout << "Creating directory: " << outputDir << endl;
        if (mkdir(outputDir.c_str(), 0777) == -1)
        {
            cerr << "Error creating directory " << outputDir << endl;
            exit(1);
        }
    }
    else if (!(info.st_mode & S_IFDIR))
    {
        cerr << outputDir << " exists but is not a directory!" << endl;
        exit(1);
    }
}

// Function to count the number of alive neighbors around a given cell
int countAliveNeighbors(const vector<vector<int>> &board, int x, int y)
{
    int aliveCount = 0;
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            aliveCount += board[x + i][y + j];
        }
    }
    return aliveCount;
}

void nextGeneration(vector<vector<int>> &currentGeneration, vector<vector<int>> &nextGeneration)
{
    int rows = currentGeneration.size();
    int cols = currentGeneration[0].size();

    // Parallelize the update of the grid with OpenMP
    #pragma omp parallel for collapse(2)
    for (int i = 1; i < rows - 1; ++i)
    {
        for (int j = 1; j < cols - 1; ++j)
        {
            int aliveNeighbors = countAliveNeighbors(currentGeneration, i, j);
            if (currentGeneration[i][j] == 1)
            {
                if (aliveNeighbors < 2 || aliveNeighbors > 3)
                    nextGeneration[i][j] = 0; // Cell dies
                else
                    nextGeneration[i][j] = 1; // Cell survives
            }
            else
            {
                if (aliveNeighbors == 3)
                    nextGeneration[i][j] = 1; // Cell becomes alive
                else
                    nextGeneration[i][j] = 0; // Cell remains dead
            }
        }
    }
}

bool isSameBoard(const vector<vector<int>> &board1, const vector<vector<int>> &board2)
{
    bool isSame = true;

    #pragma omp parallel for
    for (size_t i = 1; i < board1.size() - 1; ++i)
    {
        bool localIsSame = true;  // Local variable for this thread

        for (size_t j = 1; j < board1[i].size() - 1; ++j)
        {
            if (board1[i][j] != board2[i][j])
            {
                localIsSame = false;  // Mark local flag if boards differ
            }
        }

        // Only one thread can modify `isSame` at a time
        #pragma omp critical
        {
            if (!localIsSame)
            {
                isSame = false;
            }
        }
    }

    return isSame;
}

// Function to write the final board to a file in the output directory
void writeBoardToFile(const vector<vector<int>> &board, const string &outputDir, int generation)
{
    createOutputDirectory(outputDir);  // Ensure the output directory exists
    ofstream outFile(outputDir + "/final_board_gen_" + to_string(generation) + ".txt");
    for (size_t i = 1; i < board.size() - 1; ++i)
    {
        for (size_t j = 1; j < board[i].size() - 1; ++j)
        {
            outFile << (board[i][j] ? '*' : '.') << " ";
        }
        outFile << endl;
    }
    outFile.close();
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "Incorrect usage. Please type '" << argv[0] << " <board size> <max generations> <num threads> <output directory>'" << endl;
        return 1;
    }

    string outputDir = argv[4];
    int boardSize = stoi(argv[1]);         // Board size from command line
    int maxGenerations = stoi(argv[2]);    // Max generations from command line
    int numThreads = stoi(argv[3]);        // Number of threads from command line

    omp_set_num_threads(numThreads);       // Set the number of threads

    vector<vector<int>> board(boardSize + 2, vector<int>(boardSize + 2, 0)); // Board with ghost cells
    vector<vector<int>> nextGenerationBoard(boardSize + 2, vector<int>(boardSize + 2, 0));

    // Initialize the board with a random state
    srand(time(nullptr));
    for (int i = 1; i <= boardSize; ++i)
    {
        for (int j = 1; j <= boardSize; ++j)
        {
            board[i][j] = rand() % 2;
        }
    }

    auto start = high_resolution_clock::now(); // Start timer

    int generation = 0;    // Track current generation
    bool noChange = false; // Track if the board has changed

    while (generation < maxGenerations && !noChange)
    {
        nextGeneration(board, nextGenerationBoard); // Compute the next generation

        if (isSameBoard(board, nextGenerationBoard))
        {
            noChange = true;  // Stop if no change
        }

        board.swap(nextGenerationBoard); // Move to next generation
        generation++;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    cout << "Simulation completed in " << generation << " generations and took " << duration.count() << " ms." << endl;

    // Write the final board state to a file in the output directory
    writeBoardToFile(board, outputDir, generation);

    return 0;
}
