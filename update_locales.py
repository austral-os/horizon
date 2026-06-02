import json

def update_locale(core_path, ark_path):
    with open(core_path, 'r') as f:
        core_data = json.load(f)
    with open(ark_path, 'r') as f:
        ark_data = json.load(f)
        
    arkfm_menu = ark_data.get('arkfm', {}).get('menu', {})
    arkfm_dialog = ark_data.get('arkfm', {}).get('dialog', {})
    arkfm_properties = ark_data.get('arkfm', {}).get('properties', {})
    arkfm_messages = ark_data.get('arkfm', {}).get('messages', {})
    
    if 'file_menu' not in core_data['core']:
        core_data['core']['file_menu'] = {}
        
    for k, v in arkfm_menu.items():
        core_data['core']['file_menu'][k] = v

    # Add missing dialog items
    for k, v in arkfm_dialog.items():
        if k not in core_data['core']['dialog']:
            core_data['core']['dialog'][k] = v
            
    # Add properties
    if 'properties' not in core_data['core']:
        core_data['core']['properties'] = {}
    for k, v in arkfm_properties.items():
        core_data['core']['properties'][k] = v
        
    # Add messages
    if 'messages' not in core_data['core']:
        core_data['core']['messages'] = {}
    for k, v in arkfm_messages.items():
        core_data['core']['messages'][k] = v

    with open(core_path, 'w') as f:
        json.dump(core_data, f, indent=4, ensure_ascii=False)

update_locale('share/locales/core_en.json', 'apps/arkfm/locales/en.json')
update_locale('share/locales/core_es.json', 'apps/arkfm/locales/es.json')
