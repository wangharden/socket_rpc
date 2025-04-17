#include "test.h"
#include<thread>


int main() {
    char continueCalc='y';
    while (continueCalc =='y' || continueCalc=='Y')
    {
        Matrix<int> m1, m2;
        std::cout << "please input the first matrix " << std::endl;
        m1.readMatrix();
        std::cout << "please input the first matrix" << std::endl;
        m2.readMatrix();
        Matrix<int> sum = m1 * m2;
        if (!sum.isEmpty()) {
            std::cout << "sum: " << std::endl;
            sum.printMatrix();
        }
        std::cout << "\nContinue calculation? (y/n): ";
        std::cin >> continueCalc;
    }
    return 0;
}
