#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <ctime>
#include <random>

// Basically the mutex class
class Fork {
    volatile bool is_available; // volatile so that it definitely doesn't get optimized into nonexistence. I don't think it would since it gets changed by the program, but might as well to be safe
    // Philosopher* owner; // the philosopher currently holding the fork. Probably won't be used in the actual program but might be good for debugging

    public:
        Fork() { this->is_available = true; }

        void Take() {
            while (true) {
                if (this->is_available) { // I know there are better ways to do this (like even __sync_), but with the restrictions for this project I'm honestly just unsure whether we're allowed to use those
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

    Fork seat_block;            // added after everything else. A mutex to avoid having the same last "seat" at the table changed at the same time.
                                // calling this class Fork doesn't really make sense anymore, but I'm going to keep it.         
                                // this might actually create starvation in the way that it means only 1 philosopher can approach the table at once, but it should be fine
    public:
        Table(int n) {
            this->max_at_table = n;
            this->n_at_table = 0;
        }

        void Wait_for_a_seat() {
            while (true) {
                this->seat_block.Take();
                if (this->n_at_table < this->max_at_table) {
                    this->n_at_table++;
                    this->seat_block.Put_back();
                    return;
                }
                this->seat_block.Put_back();
                std::this_thread::yield();
            }
        }

        void Leave_table() {
            if (this->n_at_table > 0) { // in theory this should always be true, but I'm not risking it
                this->seat_block.Take();
                this->n_at_table--;
                this->seat_block.Put_back();
            }
        }
};

Table* table;
Fork* forks;
Fork cout_lock; // this SHOULD be used for controlling console output, to avoid a race condition where more than 1 philosopher tries printing at the same time
// a lock on the console output could in theory create starvation in the sense that some philosopher might never get the chance to output their state
// but internally they should still be thinking/waiting/eating, so it shouldn't actually change how the program works
// and in practice outputting takes so little time and I'm setting the times to long enough that the number of philosophers would need to be incredibly high for starvation to happen (I think)

// Class for the philosophers.
class Philosopher {  
    int id;
    volatile bool is_at_table;
    std::thread phil_thread; // I realize that phil is a name, but I don't want to have to write philosopher
    double time_thinking;
    double time_eating;
    // the time_ members are how long it'll take the philosopher to think/eat, both in milliseconds

    // Apparently using enum class instead of a plain enum causes less issues and is more safe, since they don't implicitly conver to other types and is scoped
    // (that's a lot of words and I'm not 100% sure whether it would actually matter here)
    enum class philosopher_state {
        THINKING,
        HUNGRY,
        EATING
    };

    philosopher_state state;

    public:
        static int n_philosophers;  // the total number of philosophers in the program
        Philosopher() {  } // a default empty constructor just so the array can be initialized later

        Philosopher(int id, double think, double eat) {
            this->id = id;
            this->time_thinking = think;
            this->time_eating = eat;
            this->state = philosopher_state::THINKING;
        }

        void Dine() {
            this->phil_thread = std::thread(&Philosopher::Live, this);
        }

        // this is just everything the philosopher does in the problem
        void Live() { // this is a really bad name for this function, but I don't have any better ideas
            int left_fork = this->id;
            int right_fork = (this->id + 1) % Philosopher::n_philosophers; // to make it cycle for the last philosopher

            while (true) {
                this->Set_state(philosopher_state::THINKING);
                std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(this->time_thinking));

                this->Set_state(philosopher_state::HUNGRY);

                table->Wait_for_a_seat();

                forks[left_fork].Take();
                forks[right_fork].Take();

                this->Set_state(philosopher_state::EATING);
                std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(this->time_eating));

                forks[left_fork].Put_back();
                forks[right_fork].Put_back();

                table->Leave_table();
            }
        }

        void Set_state (philosopher_state new_state) {
            this->state = new_state;
            // cout_lock.Take();
            // std::cout << "Phil " << this->id << " is " << this->Get_state_as_String() << "\n";
            // cout_lock.Put_back();
        }

        std::string Get_state_as_String() {
            switch (this->state) {
                case philosopher_state::EATING:
                    return "eating";
                case philosopher_state::HUNGRY:
                    return "hungry";
                case philosopher_state::THINKING:
                    return "thinking";
                default:
                    return "on fire";
            }
        }

        void Join() {
            this->phil_thread.join();
        }

};

int Philosopher::n_philosophers = 0;

// Function for printing the states of all philosophers.
// With this, I don't have to use cout_lock and risk a philosopher never getting the chance to output its state
// on the other hand, this will be painfully unreadable with a lot of philosophers. But the previous approach also had this problem.
void Print_phil_states(Philosopher* philosophers, int n, double time_between_updates) { // time between updates given in milliseconds
    std::string output = "";
    while (true) {
        output = "";
        //std::cout << "Philosopher States:\n--------------------\n";
        output += "Philosopher States:\n--------------------\n";
        for (int i = 0; i < n; i++) {
            //std::cout << "| " << std::setw(3) << i << " | " << std::setw(10) << philosophers[i].Get_state_as_String() << " |\n";
            output += "| " + std::to_string(i) + " | " + std::string(10 - philosophers[i].Get_state_as_String().length(), ' ') + philosophers[i].Get_state_as_String() + " |\n";
        }
        //std::cout << "--------------------\n";
        output += "--------------------\n";
        std::cout << output;
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(time_between_updates));
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) return 1; // this program should only take one argument: the number of philosophers. If there isn't exactly 1 argument, something's gone horribly wrong

    int n_philosophers = std::atoi(argv[1]);
    Philosopher::n_philosophers = n_philosophers;

    // std::cout << "You inputted: " << n_philosophers << "\n";

    forks = new Fork[n_philosophers];
    table = new Table(n_philosophers-1); // -1 to not cause a deadlock where every philosopher goes to the table, grabs one fork and waits

    Philosopher* philosophers = new Philosopher[n_philosophers];

    double  min_thinking_time = 500, max_thinking_time = 1500,
            min_eating_time = 500, max_eating_time = 1500,
            thinking_time, eating_time;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> think_dist(min_thinking_time, max_thinking_time);
    std::uniform_real_distribution<> eat_dist(min_eating_time, max_eating_time);

    for (int i = 0; i < n_philosophers; i++) {
        // thinking_time = min_thinking_time + (std::rand() / (RAND_MAX + 1.0)) * (max_thinking_time - min_thinking_time);
        // eating_time = min_eating_time + (std::rand() / (RAND_MAX + 1.0)) * (max_eating_time - min_eating_time);
        thinking_time = think_dist(gen);
        eating_time = eat_dist(gen);
        philosophers[i] = Philosopher(i, thinking_time, eating_time);
    }

    // starting the philosophers only after creating all of them to avoid anything weird happening
    // it doesn't seem fair for a philosopher to take 2 forks and start eating before another one even exists
    for (int i = 0; i < n_philosophers; i++) {
        philosophers[i].Dine();
    }

    std::thread state_thread(&Print_phil_states, philosophers, n_philosophers, 500);

    // making sure that the program doesn't finish before all of the philosopher threads do
    for (int i = 0; i < n_philosophers; i++) {
        philosophers[i].Join();
    }
    // currently, the philosophers never really finish. But that's besides the point.

    state_thread.join();

    delete[] forks;
    delete table;
    delete[] philosophers;

    return 0;
}