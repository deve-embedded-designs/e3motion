/*
 * circularVectorTS.hpp - circular, threadsafe implementation of std::vector
 *
 *  Created on: Sep 30, 2015
 *      Author: xavier.descarrega
 */

#ifndef CIRCULARVECTORTS_HPP_
#define CIRCULARVECTORTS_HPP_

#include <pthread.h>
#include <vector>
#include <cstdlib>

#include  <ugv/ebtlib/commons.h>


using namespace std;

template <class dataType>
class circularVectorTS {

	private:
		uint16_t MAX_BUFFER_ELEMENTS;
		pthread_mutex_t bufferMux;					// Main data lock
		dataType * buffer;							// Pointer to where data will be stored
		uint16_t head, tail, count;					// Pointers to data

	public:
		circularVectorTS(uint16_t length);
		~circularVectorTS();

		uint16_t headPush(char8_t);
		uint16_t headPush(char8_t *, uint16_t length);
		dataType tailPop();
		bool isEmpty();
		bool empty();

};


/*
 * Constructor
 */
template <class dataType>circularVectorTS<dataType>::circularVectorTS(uint16_t length) {
	//TODO check that length does not exceed max value
	this->head = 0;
	this->tail = 0;
	this->count = 0;
	pthread_mutex_init(&this->bufferMux, NULL);
	this->MAX_BUFFER_ELEMENTS = length;
	this->buffer = (dataType *)malloc(sizeof(dataType)*this->MAX_BUFFER_ELEMENTS);
}


/*
 * Destructor
 */
template <class dataType>circularVectorTS<dataType>::~circularVectorTS() {
	pthread_mutex_destroy(&this->bufferMux);
	free(this->buffer);
}


/*
 * headPush - insert an element to the last position. Return the number of elements in the vector if everything is okay
 *
 */
//TODo return value for overrun?, whgat to do in case of overrun?
template <class dataType> uint16_t circularVectorTS<dataType>::headPush(char8_t val) {
	uint16_t elementsNum;

	pthread_mutex_lock(&this->bufferMux);
	this->buffer[this->head] = val;
	this->count++;
	if (this->count >= this->MAX_BUFFER_ELEMENTS) {
		this->count = MAX_BUFFER_ELEMENTS;
	}
	this->head++;
	elementsNum = this->count;
	if (this->head > this->MAX_BUFFER_ELEMENTS) {
		this->head = 0;
	}
	pthread_mutex_unlock(&this->bufferMux);

	return elementsNum;
}


/*
 * headPush - multiple insertion - optimized function
 *
 */

//TODo return value for overrun?, whgat to do in case of overrun?
template <class dataType> uint16_t circularVectorTS<dataType>::headPush(char8_t * values, uint16_t length) {
	uint16_t elementsNum;

	pthread_mutex_lock(&this->bufferMux);
	for (int i = 0 ; i < length ; i++) {
		this->buffer[this->head] = values[i];
		this->head++;
		if (this->head >= this->MAX_BUFFER_ELEMENTS) {
			this->head = 0;
		}
	}
	this->count += length;
	if (this->count >= this->MAX_BUFFER_ELEMENTS) {
		this->count = MAX_BUFFER_ELEMENTS;
	}
	elementsNum = this->count;
	pthread_mutex_unlock(&this->bufferMux);

	return elementsNum;
}


/*
 * tailPop - reads the value on the tail and moves the pointer's position
 *
 */
template <class dataType> dataType circularVectorTS<dataType>::tailPop() {

	dataType val;

	pthread_mutex_lock(&this->bufferMux);
	if (this->count > 0) {
		val = this->buffer[this->tail];
		this->count--;
		this->tail++;
		if (this->tail >= this->MAX_BUFFER_ELEMENTS) {
			this->tail = 0;
		}
	} else {
		// Empty
	}
	pthread_mutex_unlock(&this->bufferMux);

	return val;
}


/*
 * isEmpty - checks whether the vector is empty
 *
 */
template <class dataType> bool circularVectorTS<dataType>::isEmpty() {
	bool empty;

	pthread_mutex_lock(&this->bufferMux);
	empty = (this->count <= 0);
	pthread_mutex_unlock(&this->bufferMux);

	return empty;

}


/*
 * empty - empties the structure
 *
 */
template <class dataType> bool circularVectorTS<dataType>::empty() {
	bool empty;

	pthread_mutex_lock(&this->bufferMux);
	this->head = 0;
	this->tail = 0;
	this->count = 0;
	free(buffer);
	this->buffer = (dataType *)malloc(sizeof(dataType)*this->MAX_BUFFER_ELEMENTS);
	pthread_mutex_unlock(&this->bufferMux);

	return empty;

}



#endif /* CIRCULARVECTORTS_HPP_ */
