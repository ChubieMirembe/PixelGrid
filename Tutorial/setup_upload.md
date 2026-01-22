## Tutorial: Uploading a Game to the PixelGrid Board

This guide explains how to upload a PixelGrid game (such as Tetris) to the Arduino.

*Prerequisites:*  
- Arduino is connected  
- Board and COM port are selected  
- Sketchbook location is already set  

---

### 1. Open Arduino IDE

Launch **Arduino IDE**.

---

### 2. Open a Game

Open the game you want to upload:

```

File → Open → Games → Tetris → Tetris.ino

```

You can replace `Tetris` with any other game folder.

---

### 3. Verify the Board Settings

Make sure the correct board is selected:


```
Tools → Board
```

Make sure the correct COM port is selected:

```
Tools → Port
```

---

### 4. Upload the Game

Click the **Upload** button (right arrow icon).

Arduino IDE will:

- Compile the game  
- Upload it to the board  
- Reset the Arduino  

When the upload finishes, the PixelGrid will start running the game.

---

### 5. Switching Games

To upload a different game:

1. Open another `.ino` file  
2. Click **Upload** again  

The new game will replace the previous one on the board.

---

### Result

Your PixelGrid board is now running the selected game.

