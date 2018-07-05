# Fluid Dynamics Application

A cross-platform, fluid dynamics simulation visualisation developed by
[Snepo Research](https://www.snepo.com) using the [Cinder Framework](https://cinder.org). This software was commissioned by Museums Victoria for an interactive digital installation at *Beyond Perception: Seeing the Unseen*, a permanent exhibition at Scienceworks in Melbourne, Australia which opened in May 2018.

![Fluid Dynamics Screenshot](https://scienceworks.s3.amazonaws.com/documentation/fluid2.jpg)

The application was designed to run on 2 high performance Windows 10 PC's each with 3 connected HD LCD screens. 
Features of the application include:

- The 2 PC's remain in sync using a network message sent from the designated "master" to a "slave"
- Simulation is optimised and tested on NVidia GTX 1080 Graphics card
- Network OSC messages are sent to the Spaical Audio Server which manages sound output
- Phycial rotary encoders placed infront of the screen communicte via Arduino hardware to the PC's via USB
- Interactions with the simulation are in the form of emitters, attractors and obstacles
- All simulation parameters are exposed via an admin GUI that can be accessed using a mouse

## Building App

C++ is difficult to compile (for legacy reasons) and has no official build system. Snepo has made a platform specific project for mac and windows. To successfully build the app check out cinder, check out the project, open the visual studio solution on windows or Xcode project on mac and press build. 

Cinder 0.9.1 needs to be at the same folder level as the project. 

```
git clone --recursive https://github.com/cinder/Cinder.git
cd cinder
git checkout release_v0.9.1
```

The folder structure will look like this, where the cinder folder has been checked out using the above steps. 

![Build Folders](https://scienceworks.s3.amazonaws.com/documentation/build-folders.png)

On windows Snepo used visual studio community 2017, but it will probably work with 2015 too. (Platform toolset v140+, c++11/14)

## Museum Setup

### Process Management

We use StayUp to monitor running processes Fluid.exe. I’ve written a small batch file for each that goes into the windows startup folder. Its job is to make sure that dropbox is started and to launch and monitor the app’s process. StayUp.exe needs to be set as “run as administrator” as it uses some of the windows system events to track things like heap allocations and memory working set, etc. This can be disabled in a pinch. The main invocation of StayUp looks like this.

```
start /D %HOMEDRIVE%%HOMEPATH%\Dropbox\scienceworks-build\FluidLeft StayUp.exe "Fluid.exe" -e -i 3600 -t 10     
      ^ "Working directory" to launch from                                     ^            ^  ^       ^
                                                            Process to monitor +            |  |       |
                                                                            Event logging  -+  |       |
                                                                     Logging interval (secs) -+|       |
                                                             Seconds to wait for a process to respond -+
                                                            before considering it dead and restarting
```

## Technical Overview

### Configuration 

The configruation file is located at assets/Config.json

Each of the configuration options is documented in the marked up example below. 

```
{
    "IsLeft" : true,                            // Is this the "left" group of 3 screens or not
    "PeerIP" : "136.154.30.198",                // If IsLeft == true, this is the IP of the "right" machine
    "OSCEndpoint" : "136.154.31.22",            // Endpoint of the audio machine receiving OSC packets
    "OSCPort" : 9001,                           // Port for the audio machine 
    "EdgeInset" : 3,                            // Width of the fake obstacle I generate to prevent going off the screen
    "SyncFrameInterval" : 600,                  // How often (in frames at 60hz) to send a sync packet to `PeerIP`
    "SceneFile" : "FluidDesigner.json",         // The name of the SceneFile in dropbox that contains the transitions
    "EncoderMappings" :                         // The obstacles / emitters the encoders control (from 0 to 6). 
    [
        [ "Emitter1", "Obs-Oval" ],             // e.g the leftmost encoder will control both Emitter1 and Obs-Oval as 
        [ "Emitter2", "Obs-Egg" ],              // defined in the SceneFile whenever either object's radius > 0.01 (i.e it's visible)
        [ "Emitter3", "Obs-Thingie" ],
        [ "Emitter4", "Obs-circle" ],
        [ "Emitter5", "Obs-FlatThingie" ],
        [ "Emitter6", "Obs-RoundedRect" ]
    ]
} 
```

### Network Communication

**Sync Packet**
Every ${Config.SyncFrameInterval} frames, the “Left” machine sends an OSC message to the “Right” machine (as specified by ${Config.PeerIP}. The OSC address is /sync and it contains a single floating point argument which is the current time of the Sequencer (which is responsible for playback of the ${Config,SceneFile}. Upon receipt of this message, the “Right” machine compares this time with its current sequencer time, and if that delta is greater than 0.5 seconds, it seeks the sequencer to this time. The reason for this allowance is to prevent popping of emitters or obstacles that may be in the process of animating when within a range that is unlikely to be perceptible by an observer

**Audio triggers**
The 4 controllable parameters exposed by the Spacial Audio Server were called Smoke, Metal, Flow, and Particles. Each of these is capable of receiving a normalised floating point value (i.e in the range 0 to 1). The OSC addresses are as follows, where ${side} is “left” or “right” depending on ${Config.IsLeft}

```
kSmokeOSCAddress     = "/bp/source_volume/FD_Smoke_${side}_48k";
kMetalOSCAddress     = "/bp/source_volume/FD_Metal_${side}_48k";
kFlowOSCAddress      = "/bp/source_volume/FD_FlowField_${side}_48k";
kParticlesOSCAddress = "/bp/source_volume/FD_Particles_${side}_48k";
```

### Admin Panels

Always accessible via the ` key. (tilde / backquote). Requires a mouse to be interacted with, however.

**Sequencer**

The sequencer edits the SceneFile as defined in the config, in the default case named - FluidDesigner.json
All emitters, attractors and obstacles are time squenced using this tool or by directly editing the JSON config. 

The options are relatively self explainitory, below are descritions of the main parameters:

- Postion - changes the X, Y Value of an object at a Time using an animation curve called Ease Curve
- Radius - the size of the object in pixels, if the radius is less than 0.01 it ceases to effect the simulation
- Force - value fed into the simulation formula to effect the fluid

![Sequencer](https://scienceworks.s3.amazonaws.com/documentation/sequencer.png)

**Fluid Simulation Settings**

The Fluid Simulation settings require an advanced understanding of fluid dynamics and complex maths calculations. Out of the box the settings work to produce a nice visualisation. Experimentation by changing one value at a time and observing the effect is the best way to understand how each paramter can be modified to created different visual output. 

Two parameter that have a significant impact on the CPU load are Simulation Scale and Jacobi Iterations, if you are running on a low spec graphics card try reducing one or both of these. 

![Fluid Simulation Settings](https://scienceworks.s3.amazonaws.com/documentation/fluid-sim-settings.png)

**Particle System Settings**

The particle system runs over the top of the fluid simulation as a separate visualtion. As such it has dedicated paramters to effect the look and feel. 

![Particle System Settings](https://scienceworks.s3.amazonaws.com/documentation/particle-system-settings.png)

**Rotary Encoder Settings**

The Rotary Encoder Software Tool is used to setup the offset of the physical handle in relation to the encoder angle. The rotary encoders can be adjusted using a custom tool supplied by the manufacturer of the AEAT-6010/6012 Magnetic Encoder. Use the mouse to drag the Zero Angle of each of the 6 Encoders. 

![Rotary Encoder](https://scienceworks.s3.amazonaws.com/documentation/rotary-encoder-settings.png)
