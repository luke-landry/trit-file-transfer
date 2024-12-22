
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <limits>

// Using namespace instead of class to group utility functions since they are stateless and no data is shared between them
namespace utils {

    // Non-template function declarations
    bool is_valid_ip_address(const std::string& ip);
    bool is_valid_port(const std::string& port);

    // Template function definitions

    /*
    Prompts for and validates user input and checks it against a vector of expected values.
    If a vector of expected values is not provided, then any valid input is allowed.
    */
    template <typename T>
    T input(const std::string& prompt, const std::vector<T>& expectedValues = {}){
        bool inputValid = false;
        T input;

        while(!inputValid){
            std::cout << prompt << ": ";
            std::cin >> input;

            // Check for input stream error (due to invalid input) and handle failure
            if(std::cin.fail()){
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input" << std::endl;
                continue;
            }

            // Check against allowed values (if any)
            if((!expectedValues.empty()) && (std::find(expectedValues.begin(), expectedValues.end(), input) == expectedValues.end())){
                std::cout << "Please enter an allowed option" << std::endl;
                continue;
            }

            inputValid = true;
        }

        return input;
    }

};