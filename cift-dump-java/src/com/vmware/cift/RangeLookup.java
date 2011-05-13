package com.vmware.cift;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.util.NavigableMap;
import java.util.TreeMap;

import java.util.List;

class Range {
	Range( int lowerAddress, int upperAddress, String _functionName)
	{
		lower = lowerAddress;
		upper = upperAddress;
		functionName = _functionName;			
	}
	public String toString(){
		String s = "[" + Integer.toString(lower,16)
				+ "-" +  Integer.toString(upper,16)
				+ "] " + functionName;
		return s;
	}
   public boolean inRange( int address ){
	   return (lower <= address)&&(address <= upper);
   }
   public int lower;
   public int upper;
   public String functionName;
   public String fileName;
   public int lineno;
};

class RangeLookup {

	//private NavigableMap<Integer, Range> rangeMap;
	private List<Range> rangeList;
	
	public RangeLookup()
	{
		//rangeMap = new TreeMap<Integer, Range>();
		rangeList = new LinkedList();
	}
	
	public void addRange( int lowaddress, int highaddress, String function )
	{
		Range newRange = new Range(lowaddress,highaddress,function);
		
		//check range first
		Range range;
		if ((range = lookupRange(lowaddress)) != null)
		{
			System.err.println("range {" + newRange.toString() + "} overlaps with existing range {" + range.toString() + "}");
			return;
		}
		if ((range = lookupRange(highaddress)) != null)
		{
			System.err.println("range {" + newRange.toString() + "} overlaps with existing range {" + range.toString() + "}");
			return;
		}
		
		int listInsertionPos = 0;
		for (listInsertionPos=0;listInsertionPos<rangeList.size();++listInsertionPos) {
			Range curr = rangeList.get(listInsertionPos);
			if ((curr != null) && (highaddress < curr.lower)) {
				//insert HERE.
				break;
			}			
		}
		rangeList.add(listInsertionPos, newRange);
		
		
		/*
		 * rangeMap.put(lowaddress,newRange );
		*/
	}
	
	public Range lookupRange( int address )
	{
		Iterator<Range> iterator = rangeList.iterator();
		while(iterator.hasNext()){
			Range range = iterator.next();
			if (range.inRange(address)){
				return range;
			}
			//else if (range.upper < address)
			//	break;			
		}
		return null;	
		
		/*// To do a lookup for some value in 'key'
		Map.Entry<Integer,Range> entry = rangeMap.floorEntry(address);
		if (entry != null)
		{
			Range range = entry.getValue();
			if ((range.lower <= address) && ( address <= range.upper )) {
				return range;
			}
		}
		return null;
		*/
	}
}
