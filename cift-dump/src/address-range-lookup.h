/*******************************************************************************
* CIFT : Compiler Instrumentation Function Tracer
*
* Copyright (c) 2011 Aaron Spear  aaron_spear@ontherock.com
*
* This is free software released under the terms of the MIT license:
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
#ifndef ADDRESS_RANGE_LOOKUP_H_
#define ADDRESS_RANGE_LOOKUP_H_

#include <vector>
#include <map>
#include <algorithm> //for_each

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

/*
 * a template class that allows you to do lookup by range.  you can declare it to manage
 * any object.
 * TODO: add iterators, better smart pointer support
 */
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

	template< typename FUNCTOR >
	FUNCTOR for_each( FUNCTOR f )
	{
		return std::for_each(rangeVector.begin(),rangeVector.end(),f);
	}

	/*
	void dump()
	{
		printf("********* RANGE TABLE ***************\n");
		unsigned index=0;
		typename RangeVector::iterator i;
		for (i=rangeVector.begin();i!=rangeVector.end();++i)
		{
			printf("%02u) %s\n",index++,i->value.toString().c_str());
		}
		printf("********* END RANGE TABLE ***************\n");
	}
	*/
};

#endif /* ADDRESS_RANGE_LOOKUP_H_ */
