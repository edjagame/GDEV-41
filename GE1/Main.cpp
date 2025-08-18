#include <iostream>
#include <cmath>

struct Data {
    int n;
    int nSquared;
    float nSqRoot;
    bool isEven;
};

void Process(Data* data, int n){
    data->n = n;
    data->nSquared = n * n;
    data->nSqRoot = sqrt(n);
    data->isEven = (n % 2 == 0);
}

void PrintData(Data* data) {
    std::cout << "n: " << data->n 
              << ", nSquared: " << data->nSquared 
              << ", nSqRoot: " << data->nSqRoot 
              << ", isEven: " << (data->isEven ? "true" : "false") 
              << std::endl;
}

int main(){
    Data dataArray[10];
    for (int i = 0; i < 10; i++) {
        Process(&dataArray[i], i);
        PrintData(&dataArray[i]);
    }
    
    int length;
    std::cout << "Enter a new integer value: ";
    std::cin >> length;
    Data* newDataArray = new Data[length];

    for (int i = 0; i < length; i++) {
        Process(&newDataArray[i], i);
        PrintData(&newDataArray[i]);
    }

    delete[] newDataArray;
    return 0;

}

