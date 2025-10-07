import pandas as pd
import re
from collections import OrderedDict

def get_file_structures(old_dict_path: str, new_acb_map_path: str) -> tuple[dict, dict, dict]:
    """
    Analyses files to determine start indices and track counts for each AWB file.
    Returns three dictionaries: old_structure, new_structure, and offset_report.
    """
    old_structure = OrderedDict()
    new_structure = OrderedDict()
    offset_report = OrderedDict()

    try:
        df_acb_map = pd.read_csv(new_acb_map_path)
        df_old_dict = pd.read_csv(old_dict_path)
        df_old_dict['index'] = pd.to_numeric(df_old_dict['index'], errors='coerce').dropna().astype(int)
    except FileNotFoundError as e:
        print(f"Error: Could not read a required file: {e}")
        return {}, {}, {}

    current_start_index = 0
    for _, row in df_acb_map.iterrows():
        awb_name = row['AwbName'].strip()
        track_count = int(row['Tracks'])
        new_structure[awb_name] = {'start_index': current_start_index, 'track_count': track_count}
        current_start_index += track_count

    current_start_index = 0
    for awb_name in new_structure.keys():
        max_index = df_old_dict[df_old_dict['targetFile'] == awb_name]['index'].max()
        if pd.notna(max_index):
            track_count = int(max_index - current_start_index + 1)
            old_structure[awb_name] = {'start_index': current_start_index, 'track_count': track_count}
            current_start_index += track_count
        else:
            old_structure[awb_name] = {'start_index': current_start_index, 'track_count': 0}

    for file_name in new_structure:
        old_s = old_structure.get(file_name, {'start_index': -1, 'track_count': 0})
        new_s = new_structure[file_name]
        offset_report[file_name] = {
            'Old Tracks': old_s['track_count'], 'New Tracks': new_s['track_count'],
            'Old Start': old_s['start_index'], 'New Start': new_s['start_index'],
            'Offset': new_s['start_index'] - old_s['start_index']
        }

    return old_structure, new_structure, offset_report

def generate_event_log(old_dict_path: str, old_struct: dict, new_struct: dict, new_files: dict) -> pd.DataFrame:
    """
    Generates a complete event log of all changes by processing old and new dictionaries separately.
    """
    events = []

    df_old = pd.read_csv(old_dict_path).dropna(subset=['CueName', 'index'])
    for _, row in df_old.iterrows():
        cue_name, old_index, old_file = row['CueName'], int(row['index']), row['targetFile']

        if old_file not in old_struct or old_file not in new_struct: continue

        old_start, new_start = old_struct[old_file]['start_index'], new_struct[old_file]['start_index']
        old_local_idx, new_track_count = old_index - old_start, new_struct[old_file]['track_count']

        status, new_index, new_file = ("REMAPPED", new_start + old_local_idx, old_file)
        if old_local_idx >= new_track_count:
            status, new_index, new_file = ("REMOVED (Shrinkage)", -1, "N/A")
            
        events.append([cue_name, old_index, old_file, new_index, new_file, status])

    for new_path, awb_name in new_files.items():
        try:
            with open(new_path, 'r', encoding='utf-8') as f:
                start_index = new_struct[awb_name]['start_index']
                for line in f:
                    match = re.match(r'(.+?)#(\d+)\s\((.*)\)', line)
                    if not match: continue
                    
                    local_index = int(match.group(2)) - 1
                    global_index = start_index + local_index
                    cues = [c.strip() for c in match.group(3).replace('[pre]', '').split(';') if c.strip()]
                    for cue in cues:
                        events.append([cue, -1, "N/A", global_index, awb_name, "ADDED (New)"])
        except FileNotFoundError:
            print(f"Warning: New file '{new_path}' not found. Skipping.")

    return pd.DataFrame(events, columns=['cueName', 'oldIndex', 'oldFile', 'newIndex', 'newFile', 'status'])

def create_new_dictionary(df_events: pd.DataFrame, new_struct: dict) -> pd.DataFrame:
    """
    Builds the dictionary with the new entries as the priority, filling in placeholders as needed.
    """
    total_tracks = sum(v['track_count'] for v in new_struct.values())
    final_dict = {i: [] for i in range(total_tracks)}

    added_events = df_events[df_events['status'] == 'ADDED (New)']
    for _, row in added_events.iterrows():
        final_dict[row['newIndex']].append({'CueName': row['cueName'], 'targetFile': row['newFile']})

    remapped_events = df_events[df_events['status'] == 'REMAPPED']
    for _, row in remapped_events.iterrows():
        idx = row['newIndex']
        if not final_dict.get(idx): # Only fill if the spot is empty
            final_dict[idx].append({'CueName': row['cueName'], 'targetFile': row['newFile']})
            
    final_records = []
    for index, cues in sorted(final_dict.items()):
        if cues:
            for cue_info in cues:
                final_records.append({'CueName': cue_info['CueName'], 'index': index, 'targetFile': cue_info['targetFile']})
        else:
            target_file = "UNKNOWN.awb"
            for file, props in new_struct.items():
                if props['start_index'] <= index < props['start_index'] + props['track_count']:
                    target_file = file
                    break
            final_records.append({'CueName': 'placeholder', 'index': index, 'targetFile': target_file})
            
    return pd.DataFrame(final_records)


def save_formatted_report(df: pd.DataFrame, offset_report: dict, output_path: str):
    """Saves the report and event log to a text file."""
    df_sorted = df.sort_values(by=['cueName', 'oldIndex', 'newIndex']).reset_index(drop=True)
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("--- Structural Change & Offset Report ---\n")
        for file, data in offset_report.items():
            f.write(f"\n[*] Analysis for: {file}\n")
            f.write(f"    - Track Count: {data['Old Tracks']} -> {data['New Tracks']} (Delta: {data['New Tracks'] - data['Old Tracks']})\n")
            if data['Offset'] != 0:
                f.write(f"    - Index Offset: {data['Offset']} (Old Start: {data['Old Start']} -> New Start: {data['New Start']})\n")
        f.write("\n" + "="*80 + "\n\n")

        f.write("--- Detailed Event Log ---\n")
        max_cue = max(df_sorted['cueName'].apply(len).max(), len('cueName'))
        max_old = max(df_sorted['oldFile'].apply(len).max(), len('oldFile'))
        max_new = max(df_sorted['newFile'].apply(len).max(), len('newFile'))
        
        f.write(f"{'cueName':<{max_cue}}  {'oldIndex':>8}  {'oldFile':<{max_old}}  ->  {'newIndex':>8}  {'newFile':<{max_new}}  {'Status'}\n")
        f.write(f"{'-'*max_cue}  {'-'*10}  {'-'*max_old}      {'-'*10}  {'-'*max_new}  {'-'*25}\n")
        
        for _, row in df_sorted.iterrows():
            f.write(f"{row['cueName']:<{max_cue}}  {str(row['oldIndex']):>8}  {row['oldFile']:<{max_old}}  ->  {str(row['newIndex']):>8}  {row['newFile']:<{max_new}}  {row['status']}\n")

    print(f"Successfully generated detailed report at: {output_path}")

if __name__ == "__main__":
    # Note that some cleanup for the new dictionary might be needed afterwards
    # particularly "Selector_0" type entries, and the virtual tracks like BGM volume and so on.
    old_dictionary_file = 'bgm_dictionary.csv'
    acb_mapping_file = 'acb_mapping.csv'
    new_files_to_process = {
        'bgm_main_cues_daima.txt': 'bgm_main.awb',
        'bgm_cnk_cues_daima.txt': 'bgm_main_Cnk_00.awb'
    }
    output_report_file = 'diff.txt'
    output_dictionary_file = 'bgm_dictionary_new.csv'

    old_struct, new_struct, offset_report = get_file_structures(old_dictionary_file, acb_mapping_file)

    if old_struct and new_struct:
        event_log_df = generate_event_log(old_dictionary_file, old_struct, new_struct, new_files_to_process)
        save_formatted_report(event_log_df, offset_report, output_report_file)
        
        new_dictionary_df = create_new_dictionary(event_log_df, new_struct)
        new_dictionary_df = new_dictionary_df[['CueName', 'targetFile', 'index']]
        new_dictionary_df.to_csv(output_dictionary_file, index=False)
        print(f"Successfully generated new dictionary at: {output_dictionary_file}")