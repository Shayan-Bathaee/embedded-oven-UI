# embedded-oven-UI

## Summary:
This is a project I developed for school in my embedded systems class. For this project, I implemented a finine state machine to create the user interface for a
toaster oven. This toaster oven can be operated through an UNO 32 board. It uses the
potentiometer to select the time and temperature, with button 3 toggling the two options.
Button 3 is also used to switch between modes, of which there are Bake, Toast, and Broil.
The UI displays different settings for different modes. For example, the temperature is
constant at 500 degrees in Broil mode, and only the time can be adjusted. When the user
has selected their cooking options, they can select button 4 to begin cooking. Once they
do so, the proper cooking components will turn on and the timer will start counting down.
Also, all 8 LEDs will be on, and they will turn off from left to right along with the
cooking timer. Once the cooking has complete, the timer will reset. 

The state machine has
5 different states: Setup, Cooking, Selector Change Pending, Reset Pending, and Alert. In the
Setup state, the user has the ability to change modes and adjust the cooking settings.
The program will enter the selector change pending state whenever the user presses down
button 3. In this state, the program checks to see if the button has been held down
longer than 1 second. If so, the UI will switch between time and temperature in bake
mode. The program enters the cooking state when the user presses button 4. While in the
cook state, if the user presses button 4 again it will enter Reset Pending. Upon doing
so, the program will check to see if the user has held the button down for longer than 1
second. If so, the program will stop the cooking process preemptively.
