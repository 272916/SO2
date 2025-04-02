#include <iostream>
#include <cstdlib> //just to have std::atoi
#include <thread>
#include <chrono>

// Basically the mutex class
class Fork {
    volatile bool is_available; // volatile so that it definitely doesn't get optimized into nonexistence. I don't think it would since it gets changed by the program, but might as well to be safe
    Philosopher* owner; // the philosopher currently holding the fork. Probably won't be used in the actual program but might be good for debugging

    public:
        Fork() { this->is_available = true; }

        void Take() {
            while (true) {
                if (this->is_available) { // I know there are theoretically better ways to do this (like even __sync_), but with the restrictions for this project I'm honestly just unsure whether we're allowed to use those
                    this->is_available = false;
                    return;
                }
                std::this_thread::yield();
            }
        }

        void Put_back() {
            this->is_available = true;
        }
};

// Basically the semaphore class
class Table {
    // count
    volatile int max_at_table;  // the maximum number of philosophers allowed to be at the table at any given time.
                                // as long as this is lower than the total number of philosophers, a deadlock should never happen (though starvation might)
                                // the actual value is pretty arbitrary (as long as it isn't something like 0 or a negative number, then it probably wouldn't work)
    volatile int n_at_table;    // this hsould always be lower or equal to max_at_table (so that we don't get every philosopher grabbing 1 fork each and waiting)
                                // this value might not actually be needed, but it might be good to have it for testing

    public:
        Table(int n) {
            this->max_at_table = n;
            this->n_at_table = 0;
        }

        void Wait_for_a_seat() {
            while (true) {
                if (this->n_at_table < this->max_at_table) {
                    this->n_at_table++;
                    return;
                }
                std::this_thread::yield();
            }
        }

        void Leave_table() {
            if (this->n_at_table > 0) this->n_at_table--;
        }
};

// Class for the philosophers.
class Philosopher {  
    static int n_philosophers;  // the total number of philosophers in the program
    int id;
    volatile bool is_at_table;

    // Apparently using enum class instead of a plain enum causes less issues and is more safe, since they don't implicitly conver to other types and is scoped
    // (that's a lot of words and I'm not 100% sure whether it would actually matter here)
    enum class philosopher_state {
        THINKING,
        HUNGRY,
        EATING
    };

    philosopher_state state;

};

int main(int argc, char* argv[]) {
    if (argc != 2) return 1; // this program should only take one argument: the number of philosophers. If there isn't exactly 1 argument, something's gone horribly wrong

    int n_philosophers = std::atoi(argv[1]);

    std::cout << "You inputted: " << n_philosophers << "\n";

    return 0;
}