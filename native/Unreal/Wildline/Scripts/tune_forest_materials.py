from pathlib import Path
script=Path(__file__).with_name('import_forest.py')
exec(compile(script.read_text().split("mapping=")[0],str(script),'exec'))
ue.log('FOREST MATERIALS COMPLETE')
