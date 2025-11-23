/*
* Homework 1, EJ Mejilla 
*
* settings.txt format:
*
* rows cols
* playerX playerY
* enemyX enemyY
*
*/

#include <iostream>
#include <vector>
#include <random>
#include <fstream>

struct Grid {
    int rows;
    int cols;
    std::vector<int> playerPos;
    std::vector<int> enemyPos;
};

std::string GridDisplay(const Grid& grid) {
    std::string display;
    for (int i = 0; i < grid.rows; ++i) {
        for (int j = 0; j < grid.cols; ++j) {
            if (i == grid.playerPos[0] && j == grid.playerPos[1]) {
                display += "P"; // Player
            } else {
                display += ".";
            } 

            if (i == grid.enemyPos[0] && j == grid.enemyPos[1]) {
                display += "E"; // Enemy
            } else {
                display += ".";
            }

            display += " ";
        }
        display += "\n";
    }
    return display;
}


// Returns true if attack is successful
bool MovePlayer(Grid& grid, std::string move) {

    if (move == "north" || move == "n") {
        grid.playerPos[0] = (grid.playerPos[0] - 1 + grid.rows) % grid.rows;
    } else if (move == "south" || move == "s") {
        grid.playerPos[0] = (grid.playerPos[0] + 1) % grid.rows;
    } else if (move == "east" || move == "e") {
        grid.playerPos[1] = (grid.playerPos[1] + 1) % grid.cols;
    } else if (move == "west" || move == "w") {
        grid.playerPos[1] = (grid.playerPos[1] - 1 + grid.cols) % grid.cols;
    } else if (move == "attack" || move == "a") {
        if (grid.playerPos == grid.enemyPos) {
            std::cout << "Enemy defeated!\n";
            return true;
        } else {
            std::cout << "Missed :-(\n";
        }
    } else {
        std::cout << "Invalid move.\n";
    }
    return false;
}

bool MoveEnemy(Grid& grid) {
    int dir = rand() % 4;

    if (grid.enemyPos == grid.playerPos) {
        std::cout << GridDisplay(grid);
        std::cout << "Enemy attacks! Game over!\n";
        return true;
    }

    switch (dir) {
        case 0: // north
            grid.enemyPos[0] = (grid.enemyPos[0] - 1 + grid.rows) % grid.rows;
            break;
        case 1: // south
            grid.enemyPos[0] = (grid.enemyPos[0] + 1) % grid.rows;
            break;
        case 2: // east
            grid.enemyPos[1] = (grid.enemyPos[1] + 1) % grid.cols;
            break;
        case 3: // west
            grid.enemyPos[1] = (grid.enemyPos[1] - 1 + grid.cols) % grid.cols;
            break;
    }

    return false;
}

Grid InitGrid(){
    Grid grid;
    std::ifstream settings("settings.txt");
    if (!settings) {
        std::cerr << "Could not open settings.txt\n";
        exit(1);
    }
    int r, c, px, py, ex, ey;
    settings >> r >> c;
    settings >> px >> py;
    settings >> ex >> ey;
    grid.rows = r;
    grid.cols = c;
    grid.playerPos = {px, py};
    grid.enemyPos = {ex, ey};
    return grid;
}

int main(){
    Grid grid = InitGrid();

    std::string move;
    bool enemyMove = false;
    while (true) {
        std::cout << GridDisplay(grid);
        std::cout << "Enter a move ('north', 'south', 'east', 'west', 'attack'), or type 'exit' to quit: ";
        std::cin >> move;
        if (move == "exit") {
            break;
        }
        if(MovePlayer(grid, move)){
            break;
        };
        
        if (enemyMove) {
            if (MoveEnemy(grid)) break;
        }
        enemyMove = !enemyMove; //enemy moves every other turn
    }
    return 0;

}