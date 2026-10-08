"""Download selected CC0 Poly Haven sources from the public asset API.

Retain source URLs, API metadata and hashes. Run with the bundled Python runtime.
"""
import concurrent.futures, hashlib, json, time, urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / 'ArtSource/Lobby/Photoreal'
MODELS = ['wooden_picnic_table','Rockingchair_01','wooden_bookshelf_worn',
          'Lantern_01','vintage_oil_lamp','wooden_axe_03','tree_stump_02',
          'dead_tree_trunk','root_cluster_01','pine_roots','moss_01',
          'bark_debris_01','wooden_bucket_01','wicker_basket_01',
          'book_encyclopedia_set_01','pot_enamel_01','wooden_bowl_01',
          'katana_stand_01','hanging_picture_frame_01','brass_goblets',
          'lantern_chandelier_01','stone_fire_pit']
TEXTURES = ['wood_floor_worn','dark_wooden_planks','jolcham_oak_bark_01',
            'rustic_stone_wall','forest_leaves_03','quatrefoil_jacquard_fabric']
HEADERS = {'User-Agent':'WildlineAssetImporter/1.0 (local game development)'}

def get_json(url):
    with urllib.request.urlopen(urllib.request.Request(url,headers=HEADERS),timeout=90) as r:
        return json.load(r)

def fetch(job):
    path, info = job
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists() and path.stat().st_size == info['size']:
        if hashlib.md5(path.read_bytes()).hexdigest()==info['md5']: return str(path)
    for attempt in range(3):
        try:
            with urllib.request.urlopen(urllib.request.Request(info['url'],headers=HEADERS),timeout=120) as r:
                data=r.read()
            assert hashlib.md5(data).hexdigest()==info['md5'], 'Download hash mismatch'
            path.write_bytes(data)
            return str(path)
        except Exception:
            if attempt==2:raise
            time.sleep(2)

def main():
    ROOT.mkdir(parents=True,exist_ok=True)
    manifest=[];jobs=[]
    for kind,ids in [('model',MODELS),('texture',TEXTURES)]:
        for asset_id in ids:
            api=get_json('https://api.polyhaven.com/files/'+asset_id)
            meta=get_json('https://api.polyhaven.com/info/'+asset_id)
            if kind=='model' and 'gltf' not in api:
                print('EXCLUDED: source has no glTF package',asset_id,flush=True)
                continue
            folder=ROOT/asset_id;folder.mkdir(exist_ok=True)
            (folder/'files.json').write_text(json.dumps(api,indent=2))
            entry={'id':asset_id,'type':kind,'page':'https://polyhaven.com/a/'+asset_id,
                   'license':'CC0-1.0','license_url':'https://polyhaven.com/license',
                   'resolution':'4k','metadata':meta,'files':[]}
            if kind=='model':
                file=api['gltf']['4k']['gltf'];name=asset_id+'_4k.gltf'
                selected={name:file,**file.get('include',{})}
            else:
                selected={}
                for channel in ['Diffuse','nor_gl','Rough','Displacement']:
                    if channel in api:
                        info=api[channel]['4k'].get('jpg') or api[channel]['4k'].get('png')
                        selected[info['url'].split('/')[-1]]=info
                assert len(selected)==4,(asset_id,'Incomplete PBR texture set',list(selected))
            for name,info in selected.items():
                jobs.append((folder/name,info));entry['files'].append({'path':name,**{k:info[k] for k in ['url','md5','size']}})
            manifest.append(entry)
            print('QUEUED',asset_id,sum(f['size'] for f in entry['files'])//1048576,'MiB',flush=True)
    (ROOT/'sources.json').write_text(json.dumps(manifest,indent=2))
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for result in pool.map(fetch,jobs):print('DOWNLOADED',result,flush=True)
    print('LOBBY ASSET DOWNLOAD COMPLETE',len(manifest),'sources',len(jobs),'files',flush=True)

if __name__=='__main__':main()
