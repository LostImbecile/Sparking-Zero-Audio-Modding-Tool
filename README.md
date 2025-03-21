# Sparking Zero Audio Modding Tool
External Programs used: [`vgmstream-cli (fork)`](https://github.com/Lostlmbecile/vgmstream-fork-dbsz), [`AcbEditor`](https://github.com/blueskythlikesclouds/SonicAudioTools), [`VGAudioCli`](https://github.com/Thealexbarney/VGAudio), [`UnrealPak`](https://github.com/RiotOreO/unrealpak),[`UnrealReZen`](https://github.com/rm-NoobInCoding/UnrealReZen)

Note that some were modified and all are Windows Exclusive, hence this tool also being <ins>Windows Only<ins>.

- Alternate Download: [GameBanana](https://gamebanana.com/tools/18312)
## 🚀 Quick Instructions

1. Put your Game's Pak directory path in the config file.
2. Get your `.AWB` and `.uasset` files with [Fmodel](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.pnuxbb3cbn2y#heading=h.qbnatqx0p168).
3. Drag any of the files into the tool. They can be located anywhere, but both files must be in the same folder.
4. **Identify the file to replace**:  
   Rename your `.wav` to match the original file name or name it "Cue_N" (N is its number)
5. Put the renamed `.wav` into the folder with the extracted HCAs.
6. Drag the folder into the tool and enter a name for the mod.

**That's it!** You can also see this [guide](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.qg24fpgvrtbx)

## 🎺BGM
Follow this [Guide](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.0#heading=h.mdona1wxpcnp), [Spanish Translation By Ekonomia](https://docs.google.com/document/d/1ZyRd0iUqMNFcSFMW1cC1FB3ntIc9PmtWDeGvDbRIAGg/edit?tab=t.0#heading=h.mdona1wxpcnp)
- You can replace all tracks in the game without the use of reloaded, and looping points can be set automatically without your interference, nor do you need to convert WAVs yourself
- You can port all of your HCAs used for reloaded directly, and you can do the rest following the guide (see [Porting From Reloaded](https://docs.google.com/document/d/1hjCoHq5XxsIRARTcqUn12roO_SVsuiYhDwmwWXCrDQ0/edit?tab=t.0#heading=h.4le3ikxk076w))


### ⚠️ Disclaimer
**Does not support se_battle, se_ui, & se_ADVIF!**

## 📝 Notes
- **File-name sensitive**: Ensure all file names match.
- Keep the generated **`.hcakey`** in the folder—it’s needed to reconstruct the original key.
- Always retain the **Tools folder** in the directory.

## 📋 Details

This tool handles all audio modding steps, including:
- Extracting `.awb` files if their corresponding `.uasset` or `.acb` files exist.
- Automatically converting all `.wav` files in the dropped folder to **HCA** format.
- Packaging folders back into `.awb` and injecting the new `.acb` into a `.uasset` file.
- Moving the packaged mod directly into your game's **mods folder**.
- Replaces BGM just like voices, and allows you to extract BGM files and listen to them directly

Contact `lostimbecile` on Discord for any issues or join the modding server: https://discord.gg/tgFrebr.

## 💻 Code Info
I didn't bother writing docs or comments for everything, nor did I structure it in a reasonable way (everything done in a hurry using tools when I could, especially for simple tasks), sorry about that, for other games you may contact me for details on what needs to be changed

There are 3 tools in this project:

- `main` **SparkingZeroAudioModdingTool**: Handles everything outside of BGM Injection and metadata addition to WAVs
   - **args:**
      - Any amount of .acb, .awb, .uasset files -> extracts the sounds into a folder, and converts them to WAV
      - Any amount of folders -> packages the sounds back into the .awb and .uasset and creates a mod (utoc/ucas/pak)
      - Any amount of .pak files -> extracts their contents into a folder
- `sub` **BgmModdingTool**: Handles BGM injection, which includes awb+uasset and index+cue mapping
   - **args:**
       - Any amount of .awb files -> extracts their headers
       - Any amount of folders -> injects them in the relevant .awb and .uasset files
       - "--extract" .awb files -> extracts the .awb file content into HCA
       - "--cmd" * -> doesn't ask the user to press enter to exit  
- `sub` **AddWavMetadata**: My rough implementation of metadata addition to WAVs
   - **args:**
      - file.wav "Title" "Album" "Artist" "Genre" "Track Number" `[All Mandatory]`     

The vgmstream I use was forked and modified: https://github.com/Lostlmbecile/vgmstream-fork-dbsz 
