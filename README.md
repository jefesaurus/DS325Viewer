# DS325Viewer
A viewer for the DS325 depth camera.

To run on a new version of ubuntu, you need to have the DepthSenseSDK installed in /opt/softkinetic/DepthSenseSDK

And, because that is closed source and apparently not updated often, you also need to add an old version of libudev to your LD_LIBRARY_PATH as descibed here:
https://ph4m.wordpress.com/2014/02/11/getting-softkinetics-depthsense-sdk-to-work-on-arch-linux/

Basically just download this: https://github.com/ph4m/DepthSenseLinux/raw/master/thirdparty/SDK/compatibility/arch/libudev-ubuntu-12.04.tar.gz

extract and then run: 

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/containing/folder

Alternatively, it works for me to just stuff the libudev so files in /opt/softkinetic/DepthSenseSDK/lib/
