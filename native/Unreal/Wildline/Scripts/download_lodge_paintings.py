"""Retain two CC0 museum images and their per-image rights metadata."""
import hashlib,json,urllib.request
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'ArtSource/Lobby/Paintings'
root.mkdir(exist_ok=True)
headers={'User-Agent':'WildlineArtImport/1.0 (local game art)'}
def fetch(url):
    with urllib.request.urlopen(urllib.request.Request(url,headers=headers),timeout=120) as r:return r.read()
report=[]
for ident,key,title in [(41941,'ClearingDitch','Clearing a Ditch'),(41949,'EncouragingHounds','Encouraging Hounds')]:
    manifest_url=f'https://manifests.collections.yale.edu/ycba/obj/{ident}'
    manifest=json.loads(fetch(manifest_url));assert title in manifest['label']['en'][0]
    canvas=manifest['items'][0]
    rights=' '.join(v for e in canvas['metadata'] for v in e['value']['en'])
    assert 'No Copyright' in rights and 'CC0' in rights
    image_url=canvas['items'][0]['items'][0]['body']['id']
    data=fetch(image_url);(root/(key+'.jpg')).write_bytes(data)
    (root/(key+'-manifest.json')).write_text(json.dumps(manifest,indent=2))
    report.append({'key':key,'title':manifest['label']['en'][0],'source':manifest['homepage'][0]['id'],'manifest':manifest_url,'download':image_url,'license':'CC0-1.0','image_rights':rights,'credit':'Yale Center for British Art, Paul Mellon Collection','sha256':hashlib.sha256(data).hexdigest(),'width':canvas['width'],'height':canvas['height']})
    print('RETAINED',key,len(data),'bytes',flush=True)
(root/'sources.json').write_text(json.dumps(report,indent=2))
