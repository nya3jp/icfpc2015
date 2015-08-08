function showErrorMessage(msg) {
    document.getElementById('errorscreen').textContent = msg;
}

function hideErrorMessage() {
    document.getElementById('errorscreen').textContent = '';
}

function hideStartScreen() {
    document.getElementById('startscreen').style.display = 'none';
}

// TODO: no global.
var GlobalGameLog = {
	problemId: null,
	seed: null,
	tag: 'handplay',
	solution: null,
	_score: 0,
}

function addMoveLog(cmd) {
    var chr = ''
    switch(cmd) {
    case 'W':
        chr = 'p';
        break;
    case 'E':
        chr = 'b';
        break;
    case 'SW':
        chr = 'a';
        break;
    case 'SE':
        chr = 'l';
        break;
    case 'RC':
        chr = 'd';
        break;
    case 'RCC':
        chr = 'k';
        break;
    }

	GlobalGameLog.solution += chr;
    document.getElementById('keyseq').textContent = JSON.stringify([GlobalGameLog]);
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
        onJsonLoaded(fp.name, json);
    };
    reader.readAsText(fp);
}

// Move newOrig to (0,0), then where |pos| moves?
function makeOrigAs(newOrig, pos) {
    var xoff = (newOrig.y&1)==1 && (pos.y&1)==0 ? +1 : 0;
    return {x: pos.x - newOrig.x - xoff, y: pos.y-newOrig.y};
}

// Move (0,0) to |newOrig|, then where |pos| moves?
function moveOrigTo(newOrig, pos) {
    var xoff = (newOrig.y&1)==1 && ((pos.y+newOrig.y)&1)==0 ? +1 : 0;
    return {x: pos.x + newOrig.x + xoff, y: pos.y+newOrig.y};
}

function readUnits(json) {
	function normalizeUnit(u) {
		var xs = u.members.map(function(mem){return mem.x;});
		var ys = u.members.map(function(mem){return mem.x;});
		var xmin = Math.min.apply(null, xs);
		var xmax = Math.max.apply(null, xs);
		var ymin = Math.min.apply(null, ys);
		var ymax = Math.max.apply(null, ys);
		var shift = function(xy){
			return makeOrigAs({x:Math.min(u.pivot.x,xmin), y:Math.min(u.pivot.y,ymin)}, xy)
		};
		return {
			xmin: 0,
			xmax: xmax - xmin,
			ymin: 0,
			ymax: ymax - ymin,
			pivot: shift(u.pivot),
			members: u.members.map(shift),
		};
	}
    return json.units.map(normalizeUnit);
}

function readRandSeq(json) {
    function calcRandSeq(mod, seed, len) {
        var seed_lo = seed & 0xffff;
        var seed_hi = seed >> 16;

        var mult_lo=20077;
        var mult_hi=16838;
        var inc=12345;
        var result = [];
        for(var i=0; i<len; ++i) {
            var rnd = seed_hi & 0x7fff;
            result.push(rnd % mod);
            var rlo_next = seed_lo * mult_lo + inc;
            var rhi_next = seed_lo * mult_hi + seed_hi * mult_lo;
            rhi_next += rlo_next >> 16;
            rlo_next &= 0xffff;
            rhi_next &= 0xffff;
            seed_lo = rlo_next;
            seed_hi = rhi_next;
        }
        return result;
    }

	if(json.sourceSeeds.length>1) {
		showErrorMessage('This JSON contains multiple seeds, but the player loads ony the first one.')
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
        board.d[y] = new Array(board.w);
        for(var x=0; x<board.w; ++x)
            board.d[y][x] = false;
    }

    json.filled.forEach(function(pos) {
        board.d[pos.y][pos.x] = true;
    });
    return board;
}

// When a JSON object is successfully loaded.
function onJsonLoaded(jsonFileName, json) {
    hideStartScreen();

	// TODO: not Global.
	GlobalGameLog.problemId = json.id
	GlobalGameLog.seed = json.sourceSeeds[0]
	GlobalGameLog.tag = 'handplay'
	GlobalGameLog.solution = ''

    var gameState = {
        board:       readBoard(json),
        randSeq:     readRandSeq(json),
        units:       readUnits(json),
        currentUnit: null,
    };

    beginGame(gameState);
}

function beginGame(gameState) {
    function loadNext() {
        gameState.currentUnit = null;
        if (gameState.randSeq.length == 0) {
            showErrorMessage('GAME CLEAR');
            sendSolution(GlobalGameLog);
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
            showErrorMessage('NEXT BLOCK CONFLICT : GAME OVER');
            sendSolution(GlobalGameLog);
            return;
        }

        gameState.currentUnit = neo;    
        render(gameState);
    }

    // Place to the center.
    function loadUnit(idx) {
        var u = gameState.units[idx];

        // TODO: verify the precise definition of 'center'
        var yoffset = -u.ymin;
        var xoffset = Math.floor((gameState.board.w - (u.xmax+1-u.xmin)) / 2) - u.xmin;
		var shift = function(xy){return {x:xy.x+xoffset, y:xy.y+yoffset}};

        return {
            members: u.members.map(shift),
            pivot: shift(u.pivot),
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
				++y;
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
        var moveCmd = null;

        var shiftFunc = function(obj) { return {x:obj.x, y:obj.y}; };
        switch(e.keyCode) {
        case VK_LEFT:
        case 70: // F
            moveCmd = 'W';
            shiftFunc = function(obj) { return {x:obj.x-1, y:obj.y}; };
            break;
        case VK_RIGHT:
        case 75:
            moveCmd = 'E';
            shiftFunc = function(obj) { return {x:obj.x+1, y:obj.y}; };
            break;
        case 66: // B, sw
            moveCmd = 'SW';
            shiftFunc = function(obj) { return {x:obj.x-1+(obj.y%2), y:obj.y+1}; };
            break;
        case 78: // N, se
            moveCmd = 'SE';
            shiftFunc = function(obj) { return {x:obj.x+(obj.y%2), y:obj.y+1}; };
            break;
        // case VK_UP:
        //     shiftFunc = function(obj) { return {x:obj.x, y:obj.y-1}; };
        //     break;
        }
        var neo = {
            members: cur.members.map(shiftFunc),
            pivot: shiftFunc(cur.pivot),
        };

        // TODO: clean up later.
        var rotFunc = function(obj) { return {x:obj.x, y:obj.y}; };
        switch(e.keyCode) {
        case VK_D: // Rotate left/counterclockwise
            moveCmd = 'RCC';
            rotFunc = function(obj) {
                var xy = makeOrigAs(neo.pivot, obj);
                var x_ = xy.x;
                var y_ = xy.y;
                var xx = x_ - Math.floor(y_ / 2);
                var zz = y_;
                var yy = -xx - zz;

                var tmp = xx;
                xx = -yy;
                yy = -zz;
                zz = -tmp;
				return moveOrigTo(neo.pivot, {x:xx+Math.floor(zz/2), y:zz});
            };
            break;
        case VK_L: // Rotate right/clockwise
            moveCmd = 'RC';
            rotFunc = function(obj) {
                var xy = makeOrigAs(neo.pivot, obj);
                var x_ = xy.x;
                var y_ = xy.y;
                var xx = x_ - Math.floor(y_ / 2);
                var zz = y_;
                var yy = -xx - zz;

                var tmp = xx;
                xx = -zz;
                zz = -yy;
                yy = -tmp;
				return moveOrigTo(neo.pivot, {x:xx+Math.floor((zz-(zz&1))/2), y:zz});
            };
            break;
        }
        neo = {
            members: neo.members.map(rotFunc),
            pivot: rotFunc(neo.pivot),
        };

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

        addMoveLog(moveCmd);
        if(isValidMove) {
            gameState.currentUnit = neo;
        } else {
            doLock();
        }
        render(gameState);
    });
}

// Render the game state to the canvas.
function render(gameState) {
    var canvas = document.getElementById('canvas');
    ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, 1000, 4000);

    var r = 15;
    var gridMul = 1.1;
    var topLeft = {x:5, y:5};
    function coordToPosition(p) {
        return {
			x: p.x * r * gridMul * 1.7 + r * Math.sqrt(3) / 2.0 * gridMul,
            y: p.y * r * gridMul * 1.5 + r * gridMul};
    }

    function drawHex(cx, cy, r) {
        ctx.beginPath();
        for (var i = 0; i < 6; ++i) {
            var d = Math.PI / 6.0;
            ctx.lineTo(cx + r * Math.cos(d + Math.PI * i / 3.0), cy + r * Math.sin(d + Math.PI * i / 3.0));
        }
        ctx.closePath();
    }

    var textBoard = gameState.board.d.map(function(line){
        return line.map(function(c){
            return c ? '*' : '-';
        });
    });
    var py=null, px=null;
    if (gameState.currentUnit) {
        gameState.currentUnit.members.forEach(function(mem){
            textBoard[mem.y][mem.x] = '#';
        });
        py = gameState.currentUnit.pivot.y;
        px = gameState.currentUnit.pivot.x;
    }

    for (var y=0; y<gameState.board.h; ++y)
    for (var x=0; x<gameState.board.w; ++x) {
        var position = coordToPosition({x: x, y: y});
        var cx = position.x + topLeft.x;
        var cy = position.y + topLeft.y;
        if ((y&1) == 1)
            cx += r * Math.sqrt(3) / 2.0 * gridMul;
        if (textBoard[y][x]=='*') {
            ctx.fillStyle = 'black';
            drawHex(cx, cy, r);
            ctx.fill();
        } else if (textBoard[y][x]=='#') {
            ctx.fillStyle = '#080';
            drawHex(cx, cy, r);
            ctx.fill();
        }
        else {
            ctx.strokeStyle = 'black';
            drawHex(cx, cy, r);
            ctx.stroke();
        }

        if (y==py && x==px) {
            ctx.fillStyle = '#8c8';
            drawHex(cx, cy, r * 0.4);
            ctx.fill();
        }
    }

	renderNextBox(gameState)
}

function renderNextBox(gameState) {
	// TODO: share code with above.
    var canvas = document.getElementById('nextbox');
    ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, 200, 4000);

    var r = 15;
    var gridMul = 1.1;
    function coordToPosition(p) {
        return {
			x: p.x * r * gridMul * 1.7 + r * Math.sqrt(3) / 2.0 * gridMul,
            y: p.y * r * gridMul * 1.5 + r * gridMul};
    }

    function drawHex(cx, cy, r) {
        ctx.beginPath();
        for (var i = 0; i < 6; ++i) {
            var d = Math.PI / 6.0;
            ctx.lineTo(cx + r * Math.cos(d + Math.PI * i / 3.0), cy + r * Math.sin(d + Math.PI * i / 3.0));
        }
        ctx.closePath();
    }

    var ox=5, oy=5
    for (var i=0; i<gameState.randSeq.length; ++i) {
        var u = gameState.units[gameState.randSeq[i]]
        u.members.forEach(function(mem){
            var x = mem.x;
            var y = mem.y;
            var pos = coordToPosition({x:x,y:y});
            var cx = pos.x + ox;
            var cy = pos.y + oy;
            if ((y&1) == 1)
                cx += r * Math.sqrt(3) / 2.0 * gridMul;
            ctx.fillStyle = '#008';
            drawHex(cx, cy, r);
            ctx.fill();
        });
        var x = u.pivot.x;
        var y = u.pivot.y;
        var pos = coordToPosition({x:x,y:y});
        var cx = pos.x + ox;
        var cy = pos.y + oy;
        if ((y&1) == 1)
           cx += r * Math.sqrt(3) / 2.0 * gridMul;
        ctx.fillStyle = '#88c'
        drawHex(cx, cy, r * 0.4);
        ctx.fill();
		oy += coordToPosition({x:0, y:u.ymax+3}).y;
    }
}

function sendSolution(resultData) {
	var str = JSON.stringify([resultData])

	var x = new XMLHttpRequest();
	x.onreadystatechange = function() {
		console.log(this);
		if (this.readyState == 4) {
		}
	};
	x.open('POST', 'http://solutions.natsubate.nya3.jp/log');
	x.setRequestHeader('Content-Type', 'application/json');
	x.send(str);
}

window.addEventListener('load', onLoad);
