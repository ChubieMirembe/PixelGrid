## Tutorial Step: Set Arduino IDE to Use the Repo as the Sketchbook

**Goal:**  
Make Arduino IDE treat your repository as the *Sketchbook* so it can detect shared libraries stored inside the repo.

---

### 1. Open Arduino IDE  
Launch Arduino IDE normally.

---

### 2. Open Preferences  
- Click **File → Preferences**  
- The Preferences window will open.

---

### 3. Set the Sketchbook Location  
Find the field labeled **Sketchbook location** and set it to:
C:\Users\your_username\source\repos\PixelGrid


You can type it manually or click **Browse** and select the `PixelGrid` folder.

---

### 4. Save the Change  
Click **OK** to apply the new location.

---

### 5. Restart Arduino IDE  
Close Arduino IDE completely, then reopen it.

---

### 6. Verify the Library Is Detected  
Go to: Sketch → Include Library


You should now see your repo library (e.g. **PixelGridCore**) in the list.

---

### 7. Use the Shared Library in Your Game  
In your game sketch

```cpp
#include <Arduino.h>
#include <PixelGridCore.h>
```
