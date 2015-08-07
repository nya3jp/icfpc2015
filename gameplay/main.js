function showErrorMessage(msg) {
    document.getElementById('errorscreen').textContent = msg;
}

function hideErrorMessage() {
    document.getElementById('errorscreen').textContent = '';
}

function hideStartScreen() {
    document.getElementById('startscreen').style.display = 'none';
}



// Main
function onLoad() {
    document.getElementById('fileinput').addEventListener('change', onFileChanged);
}

// When a file is passed by the user to the input element.
function onFileChanged() {
    hideErrorMessage();

    var fp = this.files[0];
    var reader = new FileReader();
    reader.onload = function(e) {
        try {
            json = JSON.parse(this.result);
            console.log(json);
        } catch (e) {
            showErrorMessage('JSON Load Error');
            throw e;
        }
        onJsonLoaded(json);
    };
    reader.readAsText(fp);
}

function readUnits(json) {
    return json.units;
}

function readRandSeq(json) {
    function calcRandSeq(mod, seed, len) {
        var mul=1103515245, inc=12345;
        var result = [];
        for(var i=0; i<len; ++i) {
            // TODO: majime. How to do unsigned 32-bit op in JS...?
            result.push(len % mod);
            // result.push(seed % mod);
            // seed = (seed*mul + inc) & 0xffffffff;
        }
        return result;
    }

    return calcRandSeq(
        json.units.length,
        json.sourceSeeds[0], // TODO: support multiple games.
        json.sourceLength);
}

function readBoard(json) {
    var board = {
        'h': json.height,
        'w': json.width,
        'd': new Array(json.height),
    };
    for(var y=0; y<board.h; ++y) {
        board.d[y] = new Array(board.w).fill(false);
    }

    json.filled.forEach(function(pos) {
        board.d[pos.y][pos.x] = true;
    });
    return board;
}

// When a JSON object is successfully loaded.
function onJsonLoaded(json) {
    hideStartScreen();

    var board = readBoard(json);
    var randSeq = readRandSeq(json);
    var units = readUnits(json);

    var gameState = {
        'board': board,
        'randSeq': randSeq,
        'units': units,
        'currentUnit': null,
    };

    beginGame(gameState);
}

function beginGame(gameState) {
    function loadNext() {
        gameState.currentUnit = null;
        if (gameState.randSeq.length == 0) {
            showErrorMessage('GAME CLEAR')
            return;
        }
        var nextUnitIndex = gameState.randSeq.shift();
        var neo = loadUnit(nextUnitIndex);

        var conflict = false;
        neo.members.forEach(function(mem){
            if(gameState.board.d[mem.y][mem.x])
                conflict = true;
        });
        if(conflict) {
            showErrorMessage('NEXT BLOCK CONFLICT : GAME OVER')
            return;
        }

        gameState.currentUnit = neo;    
        render(gameState);
    }

    // Place to the center.
    function loadUnit(idx) {
        var unitSrc = gameState.units[idx];

        var ymin=0xffffffff, xmin=0xffffffff, xmax=0;
        unitSrc.members.forEach(function(cell) {
            ymin = Math.min(ymin, cell.y);
            xmin = Math.min(xmin, cell.x);
            xmax = Math.max(xmax, cell.x);
        });

        // TODO: verify the precise definition of 'center'
        var yoffset = -ymin;
        var xoffset = Math.floor((gameState.board.w - (xmax-xmin)) / 2);

        var placedUnit = {
            'members': unitSrc.members.map(function(memSrc){
                return {
                    'y': memSrc.y + yoffset,
                    'x': memSrc.x + xoffset,
                }}),
            'pivot': {
                'y': unitSrc.pivot.y + yoffset,
                'x': unitSrc.pivot.x + xoffset,
            },
        };
        return placedUnit;
    }

    function doLock() {
        var cur = gameState.currentUnit;
        if (!cur)
            return;

        cur.members.forEach(function(mem){
            gameState.board.d[mem.y][mem.x] = true;
        });

        doClear();

        gameState.currentUnit = null;
        loadNext();
    }

    function doClear() {
        for(var y=gameState.board.h-1; y>=0; --y) {
            var allSet = true;
            for(var x=0; x<gameState.board.w; ++x) {
                if (!gameState.board.d[y][x])
                    allSet = false;
            }
            if(allSet) {
                for(var yy=y; yy>=0; --yy) {
                    for(var x=0; x<gameState.board.w; ++x)
                        gameState.board.d[yy][x] = (yy>0 ? gameState.board.d[yy-1][x] : false);
                }
            }
        }
    }

    loadNext();

    document.addEventListener('keydown', function(e){
        var cur = gameState.currentUnit;
        if (!cur)
            return;

        var VK_LEFT = 37;
        var VK_UP = 38;
        var VK_RIGHT = 39;
        var VK_DOWN = 40;
        var VK_A = 65;
        var VK_F = 70;
        var VK_B = 66;
        var VK_N = 78;
        var VK_K = 75;
        var VK_D = 68;
        var VK_L = 76;

        // Move
        var neo = null;

        var shiftFunc = function(obj) { return {x:obj.x, y:obj.y}; };
        switch(e.keyCode) {
        case VK_LEFT:
        case 70: // F
            shiftFunc = function(obj) { return {x:obj.x-1, y:obj.y}; };
            break;
        case VK_RIGHT:
        case 75:
            shiftFunc = function(obj) { return {x:obj.x+1, y:obj.y}; };
            break;
        case 66: // B, sw
            shiftFunc = function(obj) { return {x:obj.x-1+(obj.y%2), y:obj.y+1}; };
            break;
        case 78: // N, se
            shiftFunc = function(obj) { return {x:obj.x+(obj.y%2), y:obj.y+1}; };
            break;
        case VK_UP:
            shiftFunc = function(obj) { return {x:obj.x, y:obj.y-1}; };
            break;
        }
        var neo = {
            members: cur.members.map(shiftFunc),
            pivot: shiftFunc(cur.pivot),
        };

        // TODO: Rotate
        switch(e.keyCode) {
        case VK_D: // Rotate left
            break;
        case VK_L: // Rotate right
            break;
        }

        // Validate
        var isValidMove = true;
        neo.members.forEach(function(mem){
            if(mem.x<0 || gameState.board.w<=mem.x)
                isValidMove = false;
            else if(mem.y<0 || gameState.board.h<=mem.y)
                isValidMove = false;
            else if(gameState.board.d[mem.y][mem.x])
                isValidMove = false;
        });

        if(isValidMove) {
            gameState.currentUnit = neo;
        } else {
            doLock();
        }
        render(gameState);
    });
}

function render(gameState) {
    var board = gameState.board;

    var textBoard = gameState.board.d.map(function(line){
        return line.map(function(c){
            return c ? '*' : '-';
        });
    });

    if (gameState.currentUnit) {
        gameState.currentUnit.members.forEach(function(mem){
            textBoard[mem.y][mem.x] = '#';
        });
    }


    var boardElem = document.getElementById('board');
    var text = '';
    for(var y=0; y<board.h; ++y) {
        var line = (y%2==0 ? "" : " ");
        for(var x=0; x<board.w; ++x) {
            line += textBoard[y][x];
            line += ' ';
        }
        text += line + '\n';
    }
    boardElem.textContent = text;
}

window.addEventListener('load', onLoad);
