#include <iostream>
#include <cstdlib> //just to have std::atoi

int main(int argc, char* argv[]) {
    if (argc != 2) return 1;

    int n_philosophers = std::atoi(argv[1]);

    std::cout << "You inputted: " << n_philosophers << "\n";

    return 0;
}