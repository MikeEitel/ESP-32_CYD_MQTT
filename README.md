# ESP-32_CYD_MQTT
A display based on   ESP32-2432S024x  boards controlled by MQTT and is dual core when using sensors.
The type of board ( defined in the plattform.ini  ) alows also if the touch part of the program can get compiled or not, and witch libraries / pins have to be used.
You can compile N type also if do not want that feature.

You can define from line 11 to 18 if and witch sensors you want. The complexity of combinations is is quite high and reason that you have to make your own tests.
This snapshot is a work in process and in my eyes an ethernal story. Lib incompatibilities made me do the strange ___Lib approach that you find in another repository.

When sensors are defined they can also be displayed on screen (in this version by pressing on the lowest leftest field).

The implementation reads the sensors via second cpu. 

You can choose between all three types of screens none, capacitive or resistive. -> x= N / C or R



What is displayed gets defined from line 332 on:
// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA  APPLICATION SPECIFIC start AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
#define touch_3_2_0
with this #define you choose the layout and what is possible to show via simple mqtt messages. ALL addressing is done via predefinded numbering. 

The hole seems to be overcomplicated on first glance but follows the concept that you use predefined tiles in with you show a) a static TITLE and b) one or more variables.
As these devices are used via node-red the layout stays fix and messaging of payloads stays flexible but basic. When used as touch, the device only sends back the touched field.

An "area" divides the screen into rectangle tiles. This first struct defines the size of each tile and the position of the (lateron defined) static text.
The second struct defines static texts. The positioning is done via offsets! An area can have more than only one static text.
The third struct defines "displayed variables" on predifined positions in an area. Wicth can be more than on per area

The possible mqtt commands are found in the "callback" function from line 847 to 970.

In the  credentials.h  you define the address and network / mqtt parameters.
As this kind of display is used in different networks, in the .ino you define whitch credential file  = network to use.
I dislike DHCP in static IOT networks and the last IP digit is also used to define the mqtt name.
So in a correct credential file only the one number in   #define myIP "100"   is enough to compile another device.
