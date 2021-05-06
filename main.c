/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<math.h>
#include<stdbool.h>
#include<inttypes.h>

struct IQrecord {
    int predeterminedValue[4];
    double iValue;
    double qValue;
    double convertedIValue;
    double convertedQValue;
    int outputValue[4];
};

void readData(struct IQrecord *batch, int IQcount);
void processData(struct IQrecord *batch, int IQcount);
void driverInteract(struct IQrecord *batch, int IQcount, int *fd);
void findErrorRate(struct IQrecord *batch, int IQcount);
double decConverter(double number);
void decToBinary(int n, int *buffer);
void unsignedDectoBinary(double realNum, int fracBits, int intBits, int *output, bool isNegative);
double conversionOutput(int *output, int length);

uint16_t write_buf[8];
char read_buf[100];

int main(int argc, char **argv)
{
    int fd, IQcount;
    char option;

    printf("Welcome to this program!\n");
    //Open the device driver folder

    fd = open("/dev/tutDevice", O_RDWR); //fdopen read/write
    if (fd < 0){
        printf("Cannot open device driver file...\n");
        //return 0;
    }

    while(1){
        printf("******* Please Enter Your Options\n");
        printf("      1. Start Program\n");
        printf("      2. Exit\n");
        scanf(" %c" , &option);
        printf("Your selected option is = %c\n", option);

        if(option == '1'){
            printf("How many total I-Q values are tested?:\n");
            scanf(" %d" , &IQcount);

            struct IQrecord batch[IQcount]; //Has 4

            printf("Number of IQ Pairs Checked: %d\n", IQcount);
            readData(batch, IQcount);
            printf("Read data done.\n");
            processData(batch, IQcount);
            printf("Process data done.\n");
            driverInteract(batch, IQcount, fd);
            printf("Driver Interact done.\n");
            findErrorRate(batch, IQcount);
            printf("Error Rate found.\n");
            break;
        }
        else if(option == '2'){
            close(fd);
            exit(1);
            break;
        }
        else {
            printf("Enter valid option = %c\n", option);
        }
    }
    close(fd);


    return 0;
}

void readData(struct IQrecord *batch, int IQcount){
    int count;
    FILE *my_file = fopen("sergeyData.csv", "r"); //Read mode

    if(my_file == NULL){
        printf("Could not open data file!");
    }

    for (count = 0; count < IQcount*4; ++count)
    {
        if(count > IQcount){
            int got = fscanf(my_file, "%d", &batch[(int)floor(count/4)].predeterminedValue[count%4]);
            if (got != 3) break; // wrong number of items maybe end of file
        } else{
            int got = fscanf(my_file, "%d,%lf,%lf", &batch[(int)floor(count/4)].predeterminedValue[count%4],&batch[count].iValue,&batch[count].qValue);
            if (got != 3) break; // wrong number of items maybe end of file
        }
    }
    fclose(my_file);
}

void processData(struct IQrecord *batch, int IQcount)
{
    //Now we convert all batch values to appropriate QM.N format and then send them to the driver.
    for (int count = 0; count < IQcount; ++count)
    {
        batch[count].convertedIValue = decConverter(batch[count].iValue);
        batch[count].convertedQValue = decConverter(batch[count].qValue);
    }
}

void driverInteract(struct IQrecord *batch, int IQcount, int *fd)
{
    int binaryBufferProcessed[16];
    for (int count = 0; count < IQcount/4; ++count)
    {
        write_buf[0] = batch[count].convertedIValue;
        write_buf[1] = batch[count+1].convertedIValue;
        write_buf[2] = batch[count+2].convertedIValue;
        write_buf[3] = batch[count+3].convertedIValue;
        write_buf[4] = batch[count].convertedQValue;
        write_buf[5] = batch[count+1].convertedQValue;
        write_buf[6] = batch[count+2].convertedQValue;
        write_buf[7] = batch[count+3].convertedQValue;
        write(fd, write_buf, strlen(write_buf)+1);

        read(fd, read_buf, 1000); //Converts string to decimal then decimal to a int array [2 base 10 -> '1''0']

        memcpy(binaryBufferProcessed, read_buf, 16)


        decToBinary(atoi(read_buf[31:15]), binaryBufferProcessed);

        batch[count].outputValue[0] = binaryBufferProcessed[0];
        batch[count].outputValue[1] = binaryBufferProcessed[1];
        batch[count].outputValue[2] = binaryBufferProcessed[2];
        batch[count].outputValue[3] = binaryBufferProcessed[3];
        batch[count+1].outputValue[0] = binaryBufferProcessed[4];
        batch[count+1].outputValue[1] = binaryBufferProcessed[5];
        batch[count+1].outputValue[2] = binaryBufferProcessed[6];
        batch[count+1].outputValue[3] = binaryBufferProcessed[7];
        batch[count+2].outputValue[0] = binaryBufferProcessed[8];
        batch[count+2].outputValue[1] = binaryBufferProcessed[9];
        batch[count+2].outputValue[2] = binaryBufferProcessed[10];
        batch[count+2].outputValue[3] = binaryBufferProcessed[11];
        batch[count+3].outputValue[0] = binaryBufferProcessed[12];
        batch[count+3].outputValue[1] = binaryBufferProcessed[13];
        batch[count+3].outputValue[2] = binaryBufferProcessed[14];
        batch[count+3].outputValue[3] = binaryBufferProcessed[15];
    }
}

void findErrorRate(struct IQrecord *batch, int IQcount)
{
    int correctBits, incorrectBits, errorRate;
    for (int count = 0; count < IQcount; ++count)
    {
        if(batch[count].outputValue == batch[count].predeterminedValue){
            correctBits++;
        } else if(batch[count].outputValue != batch[count].predeterminedValue)
        {
            incorrectBits++;
        }
    }
    errorRate = incorrectBits / (incorrectBits+correctBits);
    printf("The error rate is: %lf\n", errorRate);
}

// Takes a number, number of bits to represent fraction, number of bits to represent integer.
// And outputs a binary value of size ( number of bits to represent fraction + number of bits to represent integer)
//	For example: input -> 0.5, 2 bits for integer, 3 bits for decimal then the output will be -> 00100
double decConverter(double number)
{
    int fracBits = 12;
    int intBits = 4;
    bool isNegative = false;
    int output[fracBits+intBits];

    //If the number is negative, first process it as if it was a positive. Then apply regular 2's complement notation.
    if (number < 0){
        number *= -1;
        isNegative = true;
    }

    //This function converts the decimal number to the correct binary QM.N format
    unsignedDectoBinary(number, fracBits, intBits, &output, isNegative);

    //This functions displays the converted binary QM.N as either a binary number or a decimal number
    return conversionOutput(&output,fracBits+intBits);
}

void unsignedDectoBinary(double realNum, int fracBits, int intBits, int *output, bool isNegative)
{
    double fracOutput = realNum - (int)floor(realNum);                  //Used in fractional bit computation
    double intOutput = (int)floor(realNum);                             //Used in decimal bit computation

    for(int z = fracBits; z > 0; z--){
        //Multiplies the decimal part by 2 and takes in the integer value. Then only takes the decimal part again and
        //keeps multiplying by 2 and so on until bit limit is reached.
        fracOutput = fracOutput*2;
        output[z] = (int)floor(fracOutput);
        if(fracOutput >= 1) {
            fracOutput -= 1;
        }
    }

    for(int y = fracBits+1; y <= intBits+fracBits; y++){
        //Takes only the integer part and finds the modulus of 2. Then divides by 2 and keeps going until bit limit.
        output[y] = (int)intOutput % 2;
        intOutput = intOutput/2;
    }

    int index = 0;
    bool negLoopFinished = false;

    if(isNegative) {
        //First invert all the bits (0->1 and 1->0)
        for (int loop = intBits + fracBits; loop > 0; loop--) {
            if (output[loop] == 1)
                output[loop] = 0;
            else if (output[loop] == 0)
                output[loop] = 1;
        }

        //Go through the entire int array, accounting for integer carry over when adding a '1'
        while (!negLoopFinished && index < fracBits + intBits) {
            if (output[index] == 0) {
                output[index] = 1;
                negLoopFinished = true;
            } else {
                output[index] = 0;
                index++;
            }
        }
    }
}

double conversionOutput(int *output, int length)
{
    int integerSum = 0;
    // Displays integer equivalent of the produced binary array
    for (int loop = length; loop > 0; loop--){
        integerSum += output[loop]*(pow(2,loop-1));
    }

    return integerSum;
}

void decToBinary(int n, int *buffer)
{
    // counter for binary array
    int i = 0;
    while (n > 0) {

        // storing remainder in binary array
        buffer[i] = n % 2;
        n = n / 2;
        i++;
    }
}