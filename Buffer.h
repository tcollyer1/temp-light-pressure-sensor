// TAREN COLLYER

#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "MbedLight.h"
#include "SensorData.h"
#include <iostream>


Semaphore spaceInBuffer(20);
Semaphore samplesInBuffer;

// Buffer class - for requirement 3
class Buffer {
    private:
        //Queue<SensorData, 10> buffer;

        ILED &redLED;
        SensorData buffer[10]; // Array to hold buffer items
        int front = 0, back = 0;
        int counter = 0;
        Mutex lock;
        

    public:
        Buffer(ILED &led) : redLED(led) {}

        void writeToBuffer(SensorData item) {
            // Write to FIFO buffer

            if (bufferIsFull()) {
                redLED.lightOn();
                printf("\nERROR: Buffer is full!\n\n");
            }

            else {
                // Acquire mutex lock on critical section - adding data
                lock.lock();

                time_t the_time = item.fetchDateTime();

                printf("This is what it's adding to the buffer: \nTemp: %f\nPres: %f\nLight: %f\nDate: %s", item.fetchTemperature(), item.fetchPressure(), item.fetchLightLevel(), ctime(&the_time));
                buffer[front] = item;
                counter++;
                front = (front + 1) % 20;

                // Release mutex lock
                lock.unlock();

                samplesInBuffer.release(); // Increment num. samples
                spaceInBuffer.acquire(); // Decrement space

                printf("\nFrom writeToBuffer(): There are %d items in the buffer right now\n", bufferCount());
            }
        }

        // SensorData* flushBuffer() {
        //     SensorData contents[bufferCount()];
        //     for (int i = 0; i < bufferCount(); i++) {
        //         contents[i] = readFromBuffer();
        //     }

        //     counter = 0, front = 0, back = 0;
        //     redLED.lightOff();

        //     return contents;
        // }

        bool bufferIsFull() {
            if (counter == 20) return true;
            else return false;
        }

        bool bufferIsEmpty() {
            if (front == back && counter != 20) return true;
            else return false;   
        }

        int bufferCount() {
            return counter;
        }

        SensorData readFromBuffer() {
            printf("\nFrom readFromBuffer() before read: There are %d items in the buffer right now", bufferCount());
            printf("\nTrying to read from buffer... (May be blocking)");

            samplesInBuffer.try_acquire_for(10s); // Try to decrease... if it's 0 already it's empty, blocks for 10 secs

            printf("\nBuffer's apparently not empty so it's reading\n");
            printf("Is buffer actually empty?: %d (<-- should be 0)", bufferIsEmpty());
            
            // Acquire mutex lock on critical section - removing data to read
            lock.lock();

            SensorData itemToRead = buffer[back];
            back = (back + 1) % 20;
            redLED.lightOff();
            counter--;

            // Release mutex lock
            lock.unlock();

            spaceInBuffer.acquire(); // Decrement num. samples
            samplesInBuffer.release(); // Increment space

            time_t the_time = itemToRead.fetchDateTime();
            printf("This is the item that's been read: \nTemp: %f\nPres: %f\nLight: %f\nDate: %s", itemToRead.fetchTemperature(), itemToRead.fetchPressure(), itemToRead.fetchLightLevel(), ctime(&the_time));
            printf("\nFrom readFromBuffer() after read: There are %d items in the buffer right now\n", bufferCount());

            return itemToRead;
            
        }
};