#pragma once
#include<iostream>
#include<vector>

template<typename T>
class Matrix{
    private:
        std::vector<std::vector<T>> data;
        size_t rows;
        size_t cols;
    public:
        Matrix();
        Matrix(size_t r,size_t c):rows(r),cols(c){
            data.resize(rows,std::vector<T>(cols,0));
        }
        void readMatrix();
        Matrix<T> operator+(const Matrix<T>& other) const;
        Matrix<T> operator*(const Matrix<T>& other) const;
        void printMatrix();
        bool isEmpty(){return data.empty();}
};