/*
  Use an Arduino Leonardo or Pro Micro (or similar) to emulate a
  Ultimarc I-PAC/J-PAC (https://www.ultimarc.com/ipac1.html) like
  keyboard input for connecting arcade controls to a PC/Raspberry Pi/SBC etc.
  running M.A.M.E.
  
  Note: This is not a complete emulation nor is it programable like the
  real thing.  If you have the money, buy the real thing. If you want to tinker
  and have a spare suitable Arduino device sitting idle, or want the cheapest
  solution possible, then this might work for you.
*/

#include <Keyboard.h>

// Choose one of the following:
// Two player version. (Joystick + 3 buttons) per player + 2 start + 1 coin.
// e.g. Cocktail cabinet with controls on either side, or dual controls.
// One player version. (Joystick + 6 buttons) per player + 2 start buttons.
// Comment out if you want to use the one player/joystick config.
#define TWO_JOYSTICKS

uint8_t modifier = 0;  // Modifier key state.
uint8_t modifier_prev = 0;  // Modifier key previous state.
// Nr. of the digital input [array index] to use as the modifier button.
const uint8_t modifier_input = 0;
long sample_time = 0;  // The last time the input pins were sampled
// Number msecs of consecutive samples before we consider a state change.
const uint8_t debounce_ms = 10;
// Number of inputs/keys we are monitoring and emulating.
#ifdef TWO_JOYSTICKS
const uint8_t nr_of_inputs = 17;
#else
const uint8_t nr_of_inputs = 12;
#endif

// Table for the Digital inputs to scan. Connect arcade controls to these inputs
#ifdef TWO_JOYSTICKS
// E.g. Arduino Leonardo
uint8_t input_table[nr_of_inputs] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                     A0, A1, A2, A3, A4, A5};
#else
// E.g. Sparkfun Pro Micro
uint8_t input_table[nr_of_inputs] = {3, 4, 5, 6, 7, 8, 9, 10, 14, 15, 16, A0};
#endif

uint8_t input_state[nr_of_inputs] = {0};  // Current state of the inputs.
uint8_t prev_input_state[nr_of_inputs] = {0};  // Previous state of the inputs.
// Age (milliseconds) input was last seen in current state.
long input_last_seen[nr_of_inputs] = {0};

#ifdef TWO_JOYSTICKS
// Normal (unmodified) keycodes to send with the associated input.
uint8_t norm_keycode_table[nr_of_inputs] = {'1',              // P1 start
                                            KEY_UP_ARROW,     // P1 up
                                            KEY_DOWN_ARROW,   // P1 down
                                            KEY_LEFT_ARROW,   // P1 left
                                            KEY_RIGHT_ARROW,  // P1 right
                                            KEY_LEFT_CTRL,    // P1 button 1
                                            KEY_LEFT_ALT,     // P1 button 2
                                            ' ',              // P1 button 3
                                            '2',              // P2 start
                                            'r',              // P2 up
                                            'f',              // P2 down
                                            'd',              // P2 left
                                            'g',              // P2 right
                                            'a',              // P2 button 1
                                            's',              // P2 button 2
                                            'q',              // P2 button 3
                                            '5'               // Coin A
                                           };
// The modified keycodes to send with the associated input when the modifier
// input is held.
uint8_t alt_keycode_table[nr_of_inputs] = {0,           // P1 start
                                           '`',         // P1 up
                                           'p',         // P1 down
                                           KEY_RETURN,  // P1 left
                                           KEY_TAB,     // P1 right
                                           '5',         // P1 button 1
                                           '6',         // P1 button 2
                                           0,           // P1 button 3
                                           KEY_ESC,     // P2 start
                                           0,           // P2 up
                                           0,           // P2 down
                                           0,           // P2 left
                                           0,           // P2 right
                                           0,           // P2 button 1
                                           0,           // P2 button 2
                                           0,           // P2 button 3
                                           '6'          // Coin A
                                          };
#else
// Normal (unmodified) keycodes to send with the associated input.
uint8_t norm_keycode_table[nr_of_inputs] = {'1',              // P1 start
                                            '2',              // P2 start
                                            KEY_UP_ARROW,     // P1 up
                                            KEY_DOWN_ARROW,   // P1 down
                                            KEY_LEFT_ARROW,   // P1 left
                                            KEY_RIGHT_ARROW,  // P1 right
                                            KEY_LEFT_CTRL,    // P1 button 1
                                            KEY_LEFT_ALT,     // P1 button 2
                                            ' ',              // P1 button 3
                                            KEY_LEFT_SHIFT,   // P1 button 4
                                            'Z',              // P1 button 5
                                            'X'               // P1 button 6
                                           };
// The modified keycodes to send with the associated input when the modifier
// input is held.
uint8_t alt_keycode_table[nr_of_inputs] = {'1',         // P1 start
                                           KEY_ESC,     // P2 start
                                           '`',         // P1 up
                                           'p',         // P1 down
                                           KEY_RETURN,  // P1 left
                                           KEY_TAB,     // P1 right
                                           '5',         // P1 button 1
                                           '6',         // P1 button 2
                                           '7',         // P1 button 3
                                           '8',         // P1 button 4
                                           0,           // P1 button 5
                                           0,           // P1 button 6
                                          };
#endif

void read_inputs() {
  // Update and debounce the inputs.
  uint8_t count = 0;
  uint8_t i;
  uint8_t reading;
  long tstamp;

  // Read all the inputs.
  for (i = 0; i < nr_of_inputs; i++) {
    reading = !digitalRead(input_table[i]);
    tstamp = millis();
    // Check if the reading is still the same as the recorded state.
    if (input_state[i] == reading)
      // It is the same, so update the time last seen.
      input_last_seen[i] = tstamp;
    else
      // If input has been different for longer than the debounce time
      if (input_last_seen[i] + debounce_ms < tstamp) {
        input_state[i] = reading;  // Change the state.
        input_last_seen[i] = tstamp;  // Set the new last seen time.
      }

    // Count how many buttons are currently pressed.
    if (input_state[i] != 0)
      count++;
    // Turn on the LED if button is pressed.
    if (count)
      digitalWrite(LED_BUILTIN, HIGH);
    else
      digitalWrite(LED_BUILTIN, LOW);
  }
  // Set the modifier if button 0 is held and another button is pressed.
  modifier_prev = modifier;
  modifier = input_state[modifier_input] && count > 1;

  // Release all the keys if we've changed the modifer state
  // to make sure we are in a clean/known keyboard state.
  if (modifier != modifier_prev)
    Keyboard.releaseAll();
}

void setup() {
  uint8_t i;
  for (i = 0; i < nr_of_inputs ; i++) {
    pinMode(input_table[i], INPUT);
    digitalWrite(input_table[i], HIGH);  // Turn on internal pullup resistor
  }
  pinMode(LED_BUILTIN, OUTPUT);   // For blinking the LED when a key is pressed

  Keyboard.begin();
  Keyboard.releaseAll();
  delay(200);
}

void loop() {
  uint8_t i;
  uint8_t keycode;

  read_inputs();  // Get the current input states.

  // Loop through all the inputs.
  for (i = 0; i < nr_of_inputs; i++)
    // Check if the input state has changed.
    if (input_state[i] != prev_input_state[i]) {
      // Save the input state for later comparison.
      prev_input_state[i] = input_state[i];
      // If the modifier key is held, use the alternate keycode table.
      if (modifier)
        keycode = alt_keycode_table[i];
      else
        keycode = norm_keycode_table[i];
      // Press or release the key depending on the state of the input.
      // Only send a keycode if it is non-zero.
      if (input_state[i] and keycode != 0)
        Keyboard.press(keycode);
      else
        Keyboard.release(keycode);
    }
}
