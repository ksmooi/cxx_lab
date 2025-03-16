#include <iostream>
#include <boost/signals2.hpp>

// Define slot functions that perform arithmetic operations and return float values

// Slot function to calculate the product of two floats
float product(float x, float y) { 
    return x * y; 
}

// Slot function to calculate the quotient of two floats
float quotient(float x, float y) { 
    return x / y; 
}

// Slot function to calculate the sum of two floats
float sum(float x, float y) { 
    return x + y; 
}

// Slot function to calculate the difference between two floats
float difference(float x, float y) { 
    return x - y; 
}

// Custom combiner struct to determine the maximum value returned by all connected slots
struct maximum {
    typedef float result_type; // Define the result type of the combiner as float

    // Overload the function call operator to process the results from slots
    template<typename InputIterator>
    float operator()(InputIterator first, InputIterator last) const {
        // If no slots are connected, return a default value of 0.0f
        if (first == last) return 0.0f; 
        
        // Initialize max_value with the result of the first slot
        float max_value = *first++; 
        
        // Iterate through the remaining slot results to find the maximum value
        while (first != last) {
            if (max_value < *first)
                max_value = *first; // Update max_value if a larger value is found
            ++first; // Move to the next slot result
        }
        return max_value; // Return the maximum value found
    }
};

int main() {
    // Create a Boost.Signals2 signal with the following characteristics:
    // - The signal accepts two float arguments
    // - The signal returns a float value
    // - It uses the 'maximum' combiner to process slot return values
    boost::signals2::signal<float (float, float), maximum> sig;

    // Connect the arithmetic slot functions to the signal
    sig.connect(&product);     // Connect the 'product' slot
    sig.connect(&quotient);    // Connect the 'quotient' slot
    sig.connect(&sum);         // Connect the 'sum' slot
    sig.connect(&difference);  // Connect the 'difference' slot

    // Emit the signal by calling it like a function with two float arguments
    // All connected slots are invoked with these arguments, and their return values are processed by the 'maximum' combiner
    float max_result = sig(5.0f, 3.0f);

    // Output the result obtained from the combiner, which is the maximum value returned by the slots
    std::cout << "Maximum: " << max_result << std::endl;

    return 0; // Indicate that the program ended successfully
}
