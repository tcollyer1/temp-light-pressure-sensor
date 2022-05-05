// TAREN COLLYER

#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "MbedLight.h"
#include "SensorData.h"
#include <ctime>
#include <iostream>
#include <iterator>
#include "mbed.h"

const int BUFFER_SIZE = 20;

Semaphore spaceInBuffer(BUFFER_SIZE);
Semaphore samplesInBuffer;


// Buffer class - for requirement 3
class Buffer {
    private:
        ILED &redLED;
        SensorData<float, time_t> buffer[BUFFER_SIZE]; // Array to hold buffer items
        int front = 0, back = 0;
        int counter = 0;
        Mutex lock;
        

    public:
        Buffer(ILED &led) : redLED(led) {}

        // Writes an item to the FIFO buffer.
        void writeToBuffer(SensorData<float, time_t> item) {
            // Write to FIFO buffer

            if (bufferIsFull()) {
                redLED.lightOn();
                printf("\nERROR: Buffer is full!\n\n");
            }

            else {
                // Acquire mutex lock on critical section - adding data
                lock.lock();

                buffer[front] = item;
                counter++;
                front = (front + 1) % BUFFER_SIZE;

                // Release mutex lock
                lock.unlock();

                // samplesInBuffer.release(); // Increment num. samples
                // spaceInBuffer.acquire(); // Decrement space
            }
        }

        // Returns whether buffer is full or not.
        bool bufferIsFull() {
            if (counter == BUFFER_SIZE) return true;
            else return false;
        }

        // Returns whether buffer is empty or not.
        bool bufferIsEmpty() {
            if (front == back && counter != BUFFER_SIZE) return true;
            else return false;   
        }

        // Returns the number of items currently stored in the buffer.
        int bufferCount() {
            return counter;
        }

        // Reads the first, oldest item out of the FIFO buffer.
        SensorData<float, time_t> readFromBuffer() {

            // samplesInBuffer.try_acquire_for(10s); // Try to decrease... if it's 0 already it's empty, blocks for 10 secs
            
            // Acquire mutex lock on critical section - removing data to read
            lock.lock();

            SensorData<float, time_t> itemToRead = buffer[back];
            back = (back + 1) % BUFFER_SIZE;
            redLED.lightOff();
            counter--;

            // Release mutex lock
            lock.unlock();

            // spaceInBuffer.release(); // Increment space
            // samplesInBuffer.acquire(); // Decrement num. samples

            return itemToRead;
            
        }

        SensorData<float, time_t> peekFromBuffer() {
            int idx;

            if (front == 0) {
                idx = BUFFER_SIZE - 1;
            }
            else {
                idx = front - 1;
            }

            SensorData<float, time_t> itemToPeek = buffer[idx];

            time_t date_time = itemToPeek.fetchDateTime();

            return itemToPeek;
        }
};