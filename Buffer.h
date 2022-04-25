// TAREN COLLYER

#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "MbedLight.h"
#include "SensorData.h"
#include <iostream>

// SDBlockDevice card(PB_5, PB_4, PB_3, PF_3);
Semaphore spaceInBuffer(20);
Semaphore samplesInBuffer;

// Buffer class - for requirement 3
class Buffer {
    private:
        //Queue<SensorData, 10> buffer;

        ILED &redLED;
        SensorData buff[10]; // Array to hold buffer items
        int head = 0, tail = 0;
        int pointer = 0;
        Mutex lock;
        

    public:
        Buffer(ILED &led) : redLED(led) {
            
        }

        void writeToBuffer(SensorData item) {
            // Write to FIFO buffer

            if (bufferIsFull()) {
                redLED.lightOn();
                printf("\nERROR: Buffer is full!\n\n");
            }

            else {
                // Acquire mutex lock on critical section - adding data
                lock.lock();

                buff[pointer] = item;
                pointer++;
                head = (head + 1) % 20;

                // Release mutex lock
                lock.unlock();

                samplesInBuffer.release(); // Increment num. samples
                spaceInBuffer.acquire(); // Decrement space
            }
        }

        SensorData* flushBuffer() {
            SensorData contents[bufferCount()];
            for (int i = 0; i < bufferCount(); i++) {
                contents[i] = readFromBuffer();
            }

            pointer = 0, head = 0, tail = 0;
            redLED.lightOff();

            return contents;
        }

        bool bufferIsFull() {
            if (pointer == 20) return true;
            else return false;
        }

        bool bufferIsEmpty() {
            if (head == tail) return true;
            else return false;   
        }

        int bufferCount() {
            //return buffer.count();
            return pointer;
        }

        // SensorData* readFromBuffer(SensorData *values, bool &success) {
        SensorData readFromBuffer() {

            // while (bufferIsEmpty()); // Spins for now to block...

            printf("\nTrying to read from buffer... (May be blocking)");
            samplesInBuffer.try_acquire_for(10s); // Try to decrease... if it's 0 already it's empty, blocks for 10 secs
            printf("\nBuffer's apparently not empty so it's reading.\n");
            printf("Is buffer actually empty?: %d (<-- should be 0)", bufferIsEmpty());
            
            // Acquire mutex lock on critical section - removing data to read
            lock.lock();

            SensorData itemToRead = buff[tail];
            tail = (tail + 1) % 20;
            redLED.lightOff();
            pointer--;

            // Release mutex lock
            lock.unlock();

            spaceInBuffer.acquire(); // Decrement num. samples
            samplesInBuffer.release(); // Increment space


            return itemToRead;
            
        }
};