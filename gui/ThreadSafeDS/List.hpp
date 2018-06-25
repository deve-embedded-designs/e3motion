/*
 * listTS.hpp
 *
 *  Created on: Oct 19, 2015
 *      Author: xavi
 */

#ifndef LISTTS_HPP_
#define LISTTS_HPP_

#include <pthread.h>
#include <queue>
#include <cstdlib>

#include  <ugv/ebtlib/commons.h>


using namespace std;

template <class dataType>
class listTS {

	private:
		uint16_t MAX_LIST_ELEMENTS;
		pthread_mutex_t listMux;			// Main data lock
		vector<dataType> elementsList;				// Data

	public:
		listTS(uint16_t length);
		~listTS();

		uint16_t push(dataType);
		uint16_t headPush(dataType);
		dataType front();
		dataType pop();
		uint16_t remove(uint16_t);
		uint16_t size();
		dataType at(uint16_t);
		bool isFull();
		bool clear();


};


/*
 * Constructor
 */
template <class dataType>listTS<dataType>::listTS(uint16_t length) {
	pthread_mutex_init(&this->listMux, NULL);
	this->MAX_LIST_ELEMENTS = length;
}


/*
 * Destructor
 */
template <class dataType>listTS<dataType>::~listTS() {
	pthread_mutex_destroy(&this->listMux);
}


/*
 * headPush - insert an element to the last position. Return the number of elements in the vector if everything is okay
 *
 * returns   0: push failed
 * 			>0: push success, number of elements in queue
 */
template <class dataType> uint16_t listTS<dataType>::headPush(dataType newElement) {
	uint16_t elementsNum;

	pthread_mutex_lock(&this->listMux);
	if (this->elementsList.size() < this->MAX_LIST_ELEMENTS) {
		this->elementsList.insert(this->elementsList.begin(), newElement);
		elementsNum = this->elementsList.size();
	} else {
		// Number of elements exceeded
		elementsNum = 0;
	}
	pthread_mutex_unlock(&this->listMux);

	return elementsNum;
}

/*
 * push - insert an element to the last position. Return the number of elements in the vector if everything is okay
 *
 * returns   0: push failed
 * 			>0: push success, number of elements in queue
 */
template <class dataType> uint16_t listTS<dataType>::push(dataType newElement) {
	uint16_t elementsNum;

	pthread_mutex_lock(&this->listMux);
	if (this->elementsList.size() < this->MAX_LIST_ELEMENTS) {
		this->elementsList.push_back(newElement);
		elementsNum = this->elementsList.size();
	} else {
		// Number of elements exceeded
		elementsNum = 0;
	}
	pthread_mutex_unlock(&this->listMux);

	return elementsNum;
}


/*
 * headPush - multiple insertion - optimized function
 *
 */
template <class dataType> dataType listTS<dataType>::front() {
	dataType element = NULL;

	pthread_mutex_lock(&this->listMux);
	if (this->elementsList.size() > 0) {
		element = this->elementsList.front();
	}
	pthread_mutex_unlock(&this->listMux);

	return element;
}


/*
 * tailPop - reads the value on the tail and moves the pointer's position
 *
 */
template <class dataType> dataType listTS<dataType>::pop() {

	dataType element = NULL;

	pthread_mutex_lock(&this->listMux);
	if (this->elementsList.size() > 0) {
		element = this->elementsList.front();
		this->elementsList.erase(this->elementsList.begin());
	}
	pthread_mutex_unlock(&this->listMux);

	return element;
}



/*
 *
 *
 */
template <class dataType> uint16_t listTS<dataType>::size() {

	uint16_t elementsNum;

	pthread_mutex_lock(&this->listMux);
	elementsNum = this->elementsList.size();
	pthread_mutex_unlock(&this->listMux);

	return elementsNum;
}


/*
 *
 *
 */
template <class dataType> uint16_t listTS<dataType>::remove(uint16_t index) {

	uint16_t elementsNum;

	pthread_mutex_lock(&this->listMux);
	this->elementsList.erase(this->elementsList.begin()+(index));
	elementsNum = this->elementsList.size();
	pthread_mutex_unlock(&this->listMux);

	return elementsNum;
}

/*
 *
 *
 */
template <class dataType> dataType listTS<dataType>::at(uint16_t index) {

	dataType element;

	pthread_mutex_lock(&this->listMux);
	//TODO check size?
	element = this->elementsList[index];
	pthread_mutex_unlock(&this->listMux);

	return element;
}

/*
 *
 *
 */
template <class dataType> bool listTS<dataType>::isFull() {

	bool isFull;

	pthread_mutex_lock(&this->listMux);
	isFull = (this->elementsList.size() == this->MAX_LIST_ELEMENTS);
	pthread_mutex_unlock(&this->listMux);

	return isFull;
}


#endif /* LISTTS_HPP_ */
