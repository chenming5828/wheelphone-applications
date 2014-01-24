package com.wheelphone.alarm;


import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

public class FragmentAlarm extends Fragment {
	
	private static Button btnStop;
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		
		// Inflate the layout for this fragment
		View v = inflater.inflate(R.layout.fragment_alarm, container, false);
		
		btnStop = (Button)v.findViewById(R.id.button_stop);
		
		return v;
	}
	
	public static void changeButtonColor(int c) {
		btnStop.setBackgroundColor(c);
	}
	
}
