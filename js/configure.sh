if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

if [ ! -d "griddlyjs-app/src/wasm/" ]; then
    mkdir griddlyjs-app/src/wasm/
fi

if [ ! -d "griddlyjs-app/public/js" ]; then
    mkdir griddlyjs-app/public/js/
fi

if [ ! -d "griddlyjs-app/public/resources" ]; then
    mkdir griddlyjs-app/public/resources/
fi

cp -R ../resources/games griddlyjs-app/public/resources/games
cp -R ../resources/images griddlyjs-app/public/resources/images
cp ../resources/gdy-schema.json griddlyjs-app/public/resources/gdy-schema.json