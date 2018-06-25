/*
 * queueTS.hpp - threadsafe implementation of std::queue
 *
 *  Created on: Oct 14, 2015
 *      Author: xavier.descarrega
 */

#ifndef QUEUETS_HPP_
#define QUEUETS_HPP_


#include <pthread.h>
#include <queue>
#include <cstdlib>

//#include  <ugv/ebtlib/commons.h>


using namespace std;

template <class dataType>
class queueTS {

	private:
		uint16_t MAX_QUEUE_ELEMENTS;
		pthread_mutex_t queueMux;			// Main data lock
		queue<dataType> elementsQueue;				// Data

	public:
		queueTS(uint16_t length);
		~queueTS();

		uint16_t push(dataType);
		dataType front();
		dataType pop();
		uint16_t size();
		bool empty();


};


/*
 * Constructor
 */
template <class dataType>queueTS<dataType>::queueTS(uint16_t length) {
	//TODO check max length
	pthread_mutex_init(&this->queueMux, NULL);
	this->MAX_QUEUE_ELEMENTS = length;
}


/*
 * Destructor
 */
template <class dataType>queueTS<dataType>::~queueTS() {
	pthread_mutex_destroy(&this->queueMux);
}


/*
 * headPush - insert an element to the last position. Return the number of elements in the vector if everything is okay
 *
 * returns   0: push failed
 * 			>0: push success, number of elements in queue
 */
template <class dataType> uint16_t queueTS<dataType>::push(dataType newElement) {
	uint16_t elementsNum;

	pthread_mutex_lock(&this->queueMux);
	if (this->elementsQueue.size() < this->MAX_QUEUE_ELEMENTS) {
		this->elementsQueue.push(newElement);
		elementsNum = this->elementsQueue.size();
	} else {
		// Number of elements exceeded
		elementsNum = 0;
	}
	pthread_mutex_unlock(&this->queueMux);

	return elementsNum;
}


/*
 * headPush - multiple insertion - optimized function
 *
 */
template <class dataType> dataType queueTS<dataType>::front() {
	dataType element;

	pthread_mutex_lock(&this->queueMux);
	element = this->elementsQueue.front();
	pthread_mutex_unlock(&this->queueMux);

	return element;
}


/*
 * tailPop - reads the value on the tail and moves the pointer's position, returns NULL if none
 *
 */
template <class dataType> dataType queueTS<dataType>::pop() {

	dataType element;

	pthread_mutex_lock(&this->queueMux);
	element = this->elementsQueue.front();
	this->elementsQueue.pop();
	pthread_mutex_unlock(&this->queueMux);

	return element;
}



/*
 *
 *
 */
template <class dataType> uint16_t queueTS<dataType>::size() {

	uint16_t elementsNum;

	pthread_mutex_lock(&this->queueMux);
	elementsNum = this->elementsQueue.size();
	pthread_mutex_unlock(&this->queueMux);

	return elementsNum;
}


/*
 * empty - empties the structure
 *
 */
template <class dataType> bool queueTS<dataType>::empty() {
	bool empty;

	pthread_mutex_lock(&this->queueMux);
	while (this->elementsQueue.size()) {
		this->elementsQueue.pop();
	}

	pthread_mutex_unlock(&this->queueMux);

	return empty;

}


#endif /* QUEUETS_HPP_ */
