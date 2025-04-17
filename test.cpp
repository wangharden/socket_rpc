#include "test.h"
#include <stdexcept>

template<typename T>
Matrix<T>::Matrix() {}

template<typename T>
void Matrix<T>::readMatrix() {
    size_t rows, cols;
    std::cout << "input the rows and cols:";
    std::cin >> rows >> cols;
    if (rows < 0 || cols < 0) {
        std::cout << "invalid input";
        return;
    }
    data.resize(rows*cols);
    std::cout << "please input the number:" << std::endl;
    for (size_t i = 0; i < rows*cols; i++) {
        std::cin>>data[i];
    }
}

template<typename T>
Matrix<T> Matrix<T>::operator+(const Matrix<T>& other) const {
    if (rows != other.rows || cols != other.cols) {
        throw std::invalid_argument("矩阵维度不匹配");
    };
    Matrix<T> result(rows,cols);
    for (size_t i = 0; i < rows*cols; i++) {
            result.data[i] = data[i] + other.data[i];
    }
    return result;
}

template<typename T>
Matrix<T> Matrix<T>::operator*(const Matrix<T>& other) const {
    int rows = data.size();
    int cols = data[0].size();
    int otherRows = other.data.size();
    int otherCols = other.data[0].size();
    if (cols != otherRows) {
        throw std::invalid_argument("维度不匹配");
    }
    Matrix<T> result(rows, otherCols);
    result.data.resize(rows, std::vector<T>(otherCols));
    const int BLOCK_SIZE=32;
    #pragma omp paraeel for collaspe(2)
    for(int i=0;i<rows;i+=BLOCK_SIZE){
        for(int j=0;j<otherCols;j+=BLOCK_SIZE){
            for(int ii=0;ii<std::min(i+BLOCK_SIZE,rows);++ii){
                for(int jj=0;jj<std::min(j+BLOCK_SIZE,otherCols);++jj){
                    T sum=0;
                    for(int k=0;k<cols;k++){
                        sum+=data[ii][k]+other.data[k][jj];
                    }
                result.data[ii][jj]=sum;
                }
            }
        }
    }
    
    return result;
}

template<typename T>
void Matrix<T>::printMatrix() {
    for (const auto& row : data) {
        for (T val : row) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
}

// 显式实例化模板类，这里以 int 为例
template class Matrix<int>;