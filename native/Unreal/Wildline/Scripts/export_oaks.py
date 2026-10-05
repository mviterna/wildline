from pathlib import Path
script=Path(__file__).with_name('export_forest.py').read_text()
exec(script[:script.index("open_file(Path('GrassField/")])
exec(script[script.index("open_file(Path('Broadleaf/"):script.index("open_file(Path('Pine/")])
print('OAK_EXPORT_COMPLETE',manifest)
