/*
 * address-range-lookup.h
 *
 *  Created on: May 6, 2011
 *      Author: joe
 */

#ifndef ADDRESS_RANGE_LOOKUP_H_
#define ADDRESS_RANGE_LOOKUP_H_

#include <vector>
#include <map>

template< typename R, typename T >
class RangeItem {
	public:
	typedef R range_element;
	typedef T value_type;

	RangeItem(range_element _first, range_element _last, value_type _value)
	:first(_first),last(_last),value(_value)
	{
	}
	bool operator < (const RangeItem<range_element,value_type>& o) const{
		return last < o.first;
	}
	bool operator > (const RangeItem<range_element,value_type>& o) const{
		return o.last < first;
	}

	bool operator == (const RangeItem<range_element,value_type>& o) const{
		return (first == o.first)&&(last == o.last);
	}

	bool inRange( const range_element& addr){
		return((first <= addr) && (addr <= last));
	}
	public:
	range_element first;
	range_element last;
	value_type value;
};

template< typename R, typename T >
class RangeLookup {
public:
	typedef R range_element;
	typedef T value_type;
	typedef T* value_pointer;
	typedef RangeItem< R, T> range_item;

protected:
	typedef  std::vector< range_item >  RangeVector;
	RangeVector rangeVector;
public:
	void clear(void)
	{
		rangeVector.clear();
	}
	bool insertRange( range_element first, range_element last, T value){

		range_item newRange(first,last, value);
		//iterate over the container finding the spot to insert the value.  the right spot is
		typename RangeVector::iterator i;
		for (i=rangeVector.begin();i!=rangeVector.end();++i)
		{
			if (newRange < *i)
			{
				//this is the spot to insert it.
				rangeVector.insert(i,newRange);
				return true;
			}
			else
			if (newRange == *i)
			{
				//this should NOT happen with this container
				return false;
			}
			else
			{
				//greater than.
			}
		}
		rangeVector.push_back(newRange);
		return true;
	}

	/* @return 0 if not found */
	T* findRange( range_element addr ){
		typename RangeVector::iterator i;
		for (i=rangeVector.begin();i!=rangeVector.end();++i)
		{
			if (i->inRange(addr))
			{
				return &( i->value );
			}
		}
		return 0;
	}
};

#endif /* ADDRESS_RANGE_LOOKUP_H_ */
