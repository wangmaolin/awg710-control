%awg_signal=reshape(img',[],1);
%awg_signal=awgsignal;
awg_signal=awg_signal*0.8/max(abs(awg_signal));
marker1=mod([1:size(awg_signal,1)],2)==0;
marker2=square([1:size(awg_signal,1)]*2*pi/336)==1;
marker=[marker1;marker2]';
BuildTektronixAWG710xWFM(awg_signal,marker,4*10^9,'steptest.wfm');
