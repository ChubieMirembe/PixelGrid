## Tutorial: Testing the Arduino Pixel Board

This guide shows how to verify that the PixelGrid board is working using the `pixeltest` sketch.

---

### 1. Connect the Arduino to Your Computer

Use a **USB data cable** to connect:
Arduino → USB → Computer


The power LED on the board should turn on.

If nothing lights up:
- Try a different USB cable  
- Try a different USB port  
- Make sure the board is not damaged  

---

### 2. Open Arduino IDE

Launch **Arduino IDE**.

---

### 3. Set the Correct Board Type

Go to: Tools → Board


Select your board:

| Board | Select |
|------|--------|
| Arduino Uno | Arduino Uno |
| Arduino Mega | Arduino Mega 2560 |
| Arduino Nano | Arduino Nano |

---

### 4. Select the COM Port

Go to: Tools → Port


Choose the port that appears when the Arduino is plugged in  
(example: `COM3`, `COM5`, etc.)

If no port appears:
- Reconnect the USB cable  
- Restart Arduino IDE  
- Try another USB port  

---

### 5. Set the Repo as the Sketchbook (One-Time Setup)

Go to: File → Preferences

Set **Sketchbook location** to:
C:\Users\username\source\repos\PixelGrid


Click **OK**, then restart Arduino IDE.

This allows Arduino IDE to find shared libraries in:
PixelGrid/libraries/

---

### 6. Open the Pixel Test Sketch

Open the test file used to verify the board:
File → Open → pixeltest → pixeltest.ino

This sketch is designed to confirm:

- LEDs are working  
- Pixel mapping is correct  
- Buttons (if connected) respond  

---

### 7. Upload the Test Sketch

Click the **Upload** button (right arrow icon).

Arduino IDE will:
- Compile the sketch  
- Upload it to the board  
- Reset the Arduino  

When finished, the PixelGrid should display the test pattern.

---

### 8. What a Working Board Looks Like

A working PixelGrid board should:

- Light up the LED matrix  
- Light up the 6 Digits 

If the display looks wrong:
- The LED mapping may need adjusting  
- The wiring may need checking  

---

### 9. Troubleshooting

**No lights**
- Check LED power supply  
- Check ground connection  
- Check LED data pin  

**Upload fails**
- Try another USB cable  
- Try another USB port  
- Restart Arduino IDE  


---

### Result

Your PixelGrid board is now:

- Connected  
- Powered  
- Tested  
- Ready for games  





