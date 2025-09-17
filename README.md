# Driving-Point Signal-Flow-Graph (DPSFG) Tool

An application for designing, manipulating and visualising the behaviour of LTI
(linear time-invariant) systems derived from signal flow graphs.

Transfer functions are used in the analysis  of  the  dynamic behaviour of many
single-input single-output physical processes,  typically  within the fields of
signal  processing,  communication  theory,  control   theory   and   in  micro
electronics.

Features planned:
 - Manipulation:
   + Driving-Point Signal Flow Graph (DPSFG) drawing tool.
   + Standard low order filter designer (1st and 2nd order low/high/band/notch filters)
   + Butterworth filter designer
   + Chebyshev type I and II filter designer
   + Elliptic filter designer
   + Bessel filter designer
 - Visualisation
   + Frequency response (aka Bode plot)
   + Pole-Zero diagram
   + 3D Pole-Zero diagram
   + Time domain step and impulse responses

## Building

### Ubuntu

```sh
sudo apt install libgtk-4-dev
```

### WSL

GTK tries to use Wayland by default, resulting in this error:

```
Gdk-Message: 16:43:55.505: Error 71 (Protocol error) dispatching to Wayland display.
```

Add this variable to your global environment to fix it:

```sh
export GDK_BACKEND=x11
```
