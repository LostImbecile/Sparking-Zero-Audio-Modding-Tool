# Sparking Zero Audio Modding Tool

## 🚀 Quick Instructions

1. Put your Game's Pak directory path in the config file.
2. Get your `.AWB` and `.uasset` files.
3. Drag any of the files into the tool. They can be located anywhere, but both files must be in the same folder.
4. **Identify the file to replace**:  
   Use **foobar** or enable HCA conversion to find the file you want to replace.  
   Rename your `.wav` to match the original file name (note foobar starts from 1, HCAs start from 0).
5. Put the renamed `.wav` into the folder with the extracted HCAs.
6. Drag the folder into the tool and enter a name for the mod.

**That's it!** You can also see this [guide](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.pnuxbb3cbn2y)

## 🎺BGM
Follow this [Guide](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.0#heading=h.mdona1wxpcnp), [Spanish Translation By Ekonomia](https://docs.google.com/document/d/1ZyRd0iUqMNFcSFMW1cC1FB3ntIc9PmtWDeGvDbRIAGg/edit?tab=t.0#heading=h.mdona1wxpcnp)
- You can replace all tracks in the game now  without the use of reloaded, and looping points can be set automatically without your interference, nor do you need to convert WAVs yourself
- You can port all of your HCAs used for reloaded directly, and you can do the rest following the guide (see [Porting From Reloaded](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.0#heading=h.4le3ikxk076w))
- Some menus are bugged at times


### ⚠️ Disclaimer:
**Does not support se_battle, se_ui, & se_ADVIF!**


## 📋 Details

This tool handles all audio modding steps, including:
- Extracting `.awb` files if their corresponding `.uasset` or `.acb` files exist.
- Automatically converting all `.wav` files in the dropped folder to **HCA** format.
- Packaging folders back into `.awb` and injecting the new `.acb` into a `.uasset` file.
- Moving the packaged mod directly into your game's **mods folder**.
- Replaces BGM just like voices, and allows you to extract BGM files and listen to them directly

Contact `lostimbecile` on Discord for any issues.

As for code, I didn't bother writing docs or comments for everything, sorry about that, modify the pak and utoc generators to use it with other games.

## 🔄 Process

### Extraction:
1. **Drag & Drop** any `.acb`, `.uasset`, or `.awb` files into the tool.
2. The tool **checks for matching pairs** and **extracts `.acb` from `.uasset`** if needed.
3. Duplicate pairs are **filtered automatically**, so no manual selection is required.

### Packaging & Conversion:
1. Locate the **HCA** you want to replace and put your renamed `.wav` into the corresponding folder.
2. **Drag & Drop** all folders you want to repackage into `.awb`.
3. Provide a name for your mod when prompted.
4. The tool:
   - Converts all `.wav` files in the folders to **HCA** and replaces old files.
   - Checks if the `.acb` or `.uasset` exists and extracts `.acb` if needed.
   - Packages the folder into `.awb` and **creates a backup** of the `.uasset` file.
   - Injects the new `.acb` into the `.uasset` file.
   - Uses **UnrealPak** and **UnrealReZen** to pack the files into the mods folder.


### 📝 Notes:
- **File-name sensitive**: Ensure all file names match.
- Keep the generated **`.hcakey`** in the folder—it’s needed to reconstruct the original key.
- Always retain the **Tools folder** and **`keys.csv`** in the directory.


## 💡 Example:

1. **Drop `BTCLV_0000_00_JP.uasset`** into the tool:
   - The tool checks if matching `.awb` and `.acb` files exist.
   - If `.acb` is missing, it extracts it from the `.uasset` file.
   - The tool extracts `.awb` and stores its **`.hcakey`** in a folder named `BTCLV_0000_00_JP`.

2. Put `00121_streaming.wav` into the generated folder.

3. Drag the folder into the tool.
   - The tool converts the `.wav` into **HCA** using the reconstructed key and replaces the old file.
   - The tool then packages the folder into `.awb` and injects the new `.acb` into the `.uasset` file.

4. Input a name for your mod.
   - The tool uses **UnrealPak** and **UnrealReZen** to finalise the mod and place it in the **mods folder**.
