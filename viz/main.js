var g_canvasContext;

var g_currentGame;
var g_history;
var g_inDryRun;

function showAlertMessage(msg) {
  var elem = document.getElementById('message');
  document.getElementById('message').textContent = msg;
}

// Move |newOrig| to (0,0), then where |pos| moves to?
function makeOriginAs(newOrig, pos) {
  var x = pos.x - newOrig.x;
  var y = pos.y - newOrig.y;
  if((newOrig.y & 1) == 1 && (pos.y & 1) == 0) x--;
  return {x: x, y: y};
}

// Move (0,0) to |newOrig|, then where |pos| moves to?
function moveOriginTo(newOrig, pos) {
  var x = pos.x + newOrig.x;
  var y = pos.y + newOrig.y;
  if((newOrig.y & 1) == 1 && (y & 1) == 0) x++;
  return {x: x, y: y};
}

// The (unit-local) coordinate to be moved to the origin (of board).
function determineStart(board, unit) {
  var unitBoundBox = getUnitBoundBox(unit, false);

  var unitWidth = unitBoundBox.right - unitBoundBox.left + 1;
  var leftSpace = (board.length - unitWidth) >> 1;
  return {x: unitBoundBox.left - leftSpace, y: unitBoundBox.top};
}

// Convert |unit| to something comparable with ==.
function hashUnit(unit) {
  var obj = {
    m: [].concat(unit.members).sort(function(a,b){
      if(a.x == b.x) return a.y - b.y;
        return a.x - b.x;
    }),
    p: unit.pivot,
  };
  return JSON.stringify(obj);
}

function cloneGame(game) {
  var newGame = {};
  newGame.source = [].concat(game.source);
  newGame.board = cloneBoard(game.board);
  newGame.configurations = game.configurations;
  newGame.unit = game.unit ? cloneUnit(game.unit) : undefined;
  newGame.score = game.score;
  newGame.ls_old = game.ls_old;
  newGame.commandHistory = game.commandHistory;
  newGame.done = game.done;
  newGame.seed = game.seed;
  newGame.currentUnitHistory = [].concat(game.currentUnitHistory);
  return newGame;
}

function cloneUnit(unit) {
  var newUnit = {members: [], pivot: {}};
  for (var i = 0; i < unit.members.length; ++i) {
    newUnit.members.push({x: unit.members[i].x, y: unit.members[i].y});
  }
  newUnit.pivot.x = unit.pivot.x;
  newUnit.pivot.y = unit.pivot.y;
  return newUnit;
}

function moveUnit(unit, op) {
  for (var i = 0; i < unit.members.length; ++i) {
    op(unit.members[i]);
  }
  op(unit.pivot);
}

function inBoard(board, position) {
  return position.x >= 0 && position.x < board.length &&
    position.y >= 0 && position.y < board[0].length;
}

function isInvalidPosition(board, position) {
  return !inBoard(board, position) ||
    board[position.x][position.y] != 0;
}

function isInvalidUnitPlacement(board, unit) {
  for (var i = 0; i < unit.members.length; ++i) {
    if (isInvalidPosition(board, unit.members[i])) {
      return true;
    }
  }
  return false;
}

function rotateClockwise(position) {
  var xx = position.x - (position.y-(position.y&1))/2;
  var zz = position.y;
  var yy = -xx - zz;

  var tmp = xx;
  xx = -zz;
  zz = -yy;
  yy = -tmp;
  position.x = xx + (zz-(zz&1))/2;
  position.y = zz;
}

function rotateCounterClockwise(position) {
  var xx = position.x - (position.y-(position.y&1))/2;
  var zz = position.y;
  var yy = -xx - zz;

  var tmp = xx;
  xx = -yy;
  yy = -zz;
  zz = -tmp;
  position.x = xx + (zz-(zz&1))/2;
  position.y = zz;
}

function moveClockwise(origin, position) {
  var moved = makeOriginAs(origin, position);
  rotateClockwise(moved);
  moved = moveOriginTo(origin, moved);

  position.x = moved.x;
  position.y = moved.y;
}

function moveCounterClockwise(origin, position) {
  var moved = makeOriginAs(origin, position);
  rotateCounterClockwise(moved);
  moved = moveOriginTo(origin, moved);

  position.x = moved.x;
  position.y = moved.y;
}

function moveW(position) {
  position.x -= 1;
}

function moveSW(position) {
  if (position.y % 2 == 0) {
    position.x -= 1;
    position.y += 1;
  } else {
    position.y += 1;
  }
}

function moveSE(position) {
  if (position.y % 2 == 0) {
    position.y += 1;
  } else {
    position.x += 1;
    position.y += 1;
  }
}

function moveE(position) {
  position.x += 1;
}

function getUnitBoundBox(unit, includePivot) {
  var members = unit.members;

  var right = 0;
  var top = Infinity;
  var left = Infinity;
  var bottom = 0;

  for (var j = 0; j < members.length; ++j) {
    right = Math.max(right, members[j].x);
    top = Math.min(top, members[j].y);
    left = Math.min(left, members[j].x);
    bottom = Math.max(bottom, members[j].y);
  }

  if (includePivot) {
    var pivot = unit.pivot;

    right = Math.max(right, pivot.x);
    top = Math.min(top, pivot.y);
    left = Math.min(left, pivot.x);
    bottom = Math.max(bottom, pivot.y);
  }

  return {top: top, right: right, left: left, bottom: bottom};
}

function drawUnit(unit, topLeft, r, gridMul, context) {
  var unitBoundBox = getUnitBoundBox(unit, true);
  var board = createBoard(unitBoundBox.right + 1, unitBoundBox.bottom + 1);
  placeUnit(board, unit, true, false, true);
  drawBoard(board, r, gridMul, topLeft, context, false);
  return coordToPosition({x: 0, y: unitBoundBox.bottom + 1}, r, gridMul).y;
}

function getDrawUnitsBoundBox(units, r, gridMul, margin) {
  var totalWidth = 0;
  var totalHeight = margin.y * 2;

  for (var i = 0; i < units.length; ++i) {
    var members = units[i].members;

    var width = 0;
    var height = 0;

    for (var j = 0; j < members.length; ++j) {
      width = Math.max(width, members[j].x);
      height = Math.max(height, members[j].y);
    }

    var pivot = units[i].pivot;

    width = Math.max(width, pivot.x);
    height = Math.max(height, pivot.y);

    width += 1;
    height += 1;

    var unitSize = coordToPosition({x: width, y: height}, r, gridMul);

    totalWidth = Math.max(totalWidth, unitSize.x);
    totalHeight += unitSize.y + 10;
  }

  return {width: totalWidth + margin.x * 2, height: totalHeight};
}

function drawUnits(units, r, gridMul, topLeft, margin, context) {
  var top = margin.x;
  var left = margin.y;

  for (var i = 0; i < units.length; ++i) {
    var unit = units[i];

    context.fillText(i,
                     topLeft.x + left, topLeft.y + top + 10);
    top += 10;

    top += drawUnit(unit,
                    {x: topLeft.x + left, y: topLeft.y + top}, r, gridMul,
                    context);
  }
}

function createBoard(width, height) {
  var board = [];
  for (var x = 0; x < width; ++x) {
    var col = [];
    for (var y = 0; y < height; ++y) {
      col.push(0);
    }
    board.push(col);
  }
  return board;
}

function doClear(board, unit) {
  var rowsSet = {};
  for (var i = 0; i < unit.members.length; ++i) {
    rowsSet[unit.members[i].y] = true;
  }

  var newBoard = createBoard(board.length, board[0].length);

  var yy = board[0].length - 1;
  var ls = 0;
  for (var y = board[0].length - 1; y >= 0; --y) {
    if (rowsSet[y]) {
      var cleared = true;
      for (var x = 0; x < board.length; ++x) {
        if (board[x][y] == 0) {
          cleared = false;
          break;
        }
      }
      if (cleared) {
        ls += 1;
        continue;
      }
    }

    for (var x = 0; x < board.length; ++x) {
      newBoard[x][yy] = board[x][y];
    }
    --yy;
  }

  return {ls: ls, board: newBoard};
}

function doLock() {
  var unit = g_currentGame.unit;

  var size = unit.members.length;

  var result = doClear(g_currentGame.board, unit);
  var ls = result.ls;

  var points = size + 100 * (1 + ls) * ls / 2;
  var line_bonus = g_currentGame.ls_old > 1 ?
    Math.floor((g_currentGame.ls_old - 1) * points / 10) : 0;
  var move_score = points + line_bonus;

  g_currentGame.ls_old = ls;

  g_currentGame.score += move_score;
  updateInfo();

  g_currentGame.board = result.board;
}

function showManual() {
  document.getElementById('manual').style.display = "block";
  document.getElementById('showManualDiv').style.display = "none";
}

function hideManual() {
  document.getElementById('manual').style.display = "none";
  document.getElementById('showManualDiv').style.display = "block";
}

function updateInfo() {
  if (g_currentGame === undefined) {
    return;
  }

  var infoDiv = document.getElementById("info");
  infoDiv.innerText =
    'Score: ' + g_currentGame.score + '\n' +
    'Remaining units: ' + g_currentGame.source.length + '\n' +
    'Command length: ' + g_currentGame.commandHistory.length;
}

function undo() {
  if (g_history.length > 0) {
    g_currentGame = g_history.pop();
    showAlertMessage(''); // clear
  }
  updateInfo();
}

function undoAll() {
  if (g_history.length > 1) {
    g_currentGame = g_history[1];
    g_history = [g_history[0]];
  }
  updateInfo();
}

function handleKey(e) {
  var keyCode = e.keyCode;

  if (keyCode == 'U'.charCodeAt(0)) {
    undo();

    drawGame(undefined);
    logKey();
    return;
  }

  var command;

  switch (keyCode) {
  case 'W'.charCodeAt(0):
    command = 'k';
    break;

  case 'E'.charCodeAt(0):
    command = 'd';
    break;

  case 'A'.charCodeAt(0):
    command = 'p';
    break;

  case 'Z'.charCodeAt(0):
    command = 'a';
    break;

  case 'X'.charCodeAt(0):
    command = 'l';
    break;

  case 'D'.charCodeAt(0):
    command = 'b';
    break;

  default:
    return;
  }

  doCommand(command, e.shiftKey);

  logKey();
}

function doCommand(command, dryRun) {
  if (g_currentGame === undefined ||
      g_currentGame.unit === undefined) {
    return;
  }

  var newUnit = cloneUnit(g_currentGame.unit);

  switch (command) {
  case 'k':
  case 's':
  case 't':
  case 'u':
  case 'w':
  case 'x':
    moveUnit(newUnit, moveCounterClockwise.bind(undefined, newUnit.pivot));
    break;

  case 'd':
  case 'q':
  case 'r':
  case 'v':
  case 'z':
  case '1':
    moveUnit(newUnit, moveClockwise.bind(undefined, newUnit.pivot));
    break;

  case 'p':
  case "'":
  case '!':
  case '.':
  case '0':
  case '3':
    moveUnit(newUnit, moveW);
    break;

  case 'a':
  case 'g':
  case 'h':
  case 'i':
  case 'j':
  case '4':
    moveUnit(newUnit, moveSW);
    break;

  case 'l':
  case 'm':
  case 'n':
  case 'o':
  case ' ':
  case '5':
    moveUnit(newUnit, moveSE);
    break;

  case 'b':
  case 'c':
  case 'e':
  case 'f':
  case 'y':
  case '2':
    moveUnit(newUnit, moveE);
    break;

  default:
    return;
  }

  if (dryRun) {
    g_inDryRun = true;
    drawGame(newUnit);
    return;
  }

  var newUnitHash = hashUnit(newUnit);
  if (g_currentGame.currentUnitHistory.indexOf(newUnitHash) != -1) {
    showAlertMessage("ALREADY VISITED!!");
    return;
  } else {
    showAlertMessage(''); // clear
  }

  saveGame();
  g_currentGame.currentUnitHistory.push(newUnitHash);
  g_currentGame.commandHistory += command;
  if (!isInvalidUnitPlacement(g_currentGame.board, newUnit)) {
    g_currentGame.unit = newUnit;
  } else {
    placeUnit(g_currentGame.board, g_currentGame.unit, false);
    doLock();
    g_currentGame.unit = undefined;
  }
  drawGame(undefined);
}

function drawSelectedProblem() {
  var name = problems.options[problems.selectedIndex].value;
  drawProblem(name);
  window.location.hash = name;
}

function extractFromFragment() {
  for (var i = 0; i < problems.options.length; ++i) {
    if (problems.options[i].value == window.location.hash.substr(1)) {
      problems.selectedIndex = i;
      return;
    }
  }
}

function init() {
  var problems = document.getElementById('problems');
  for (var i = 0; i < 24; ++i) {
    var file = 'problem_' + i + '.json';
    var option = document.createElement('option');
    option.value = file;
    option.innerText = file;
    problems.appendChild(option);
  }
  for (var i = 0; i < 100; ++i) {
    var file = 'test_' + i + '.json';
    var option = document.createElement('option');
    option.value = file;
    option.innerText = file;
    problems.appendChild(option);
  }

  problems.addEventListener('change', function () {
    drawSelectedProblem();
  });

  window.addEventListener('hashchange', function () {
    extractFromFragment();
    drawSelectedProblem();
  });

  extractFromFragment();
  drawSelectedProblem();

  document.body.addEventListener('keydown', function (e) {
    if (e.target == document.getElementById('log')) {
      return true;
    }
    handleKey(e);
  });
  document.body.addEventListener('keyup', function (e) {
    if (e.target == document.getElementById('log')) {
      return true;
    }
    if (g_inDryRun) {
      drawGame(undefined);
    }
  });

  var logDiv = document.getElementById('log');
  logDiv.addEventListener('change', function () {
    var commands = logDiv.value;
    undoAll();
    for (var i = 0; i < commands.length; ++i) {
      console.log(commands[i]);
      doCommand(commands[i]);
    }
  });
}

function cloneBoard(board) {
  var width = board.length;
  var height = board[0].length;

  var newBoard = [];

  for (var x = 0; x < width; ++x) {
    var col = [];
    for (var y = 0; y < height; ++y) {
      col.push(board[x][y]);
    }
    newBoard.push(col);
  }
  return newBoard;
}

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

function drawGame(dryUnit) {
  if (g_currentGame.unit === undefined) {
    if (g_currentGame.source.length > 0) {
      var newUnit = cloneUnit(g_currentGame.configurations.units[g_currentGame.source.shift()]);
      var newOrigin = determineStart(g_currentGame.board, newUnit);
      moveUnit(newUnit, function (position) {
        var newp = makeOriginAs(newOrigin, position);
        position.x = newp.x;
        position.y = newp.y;
      });

      if (!isInvalidUnitPlacement(g_currentGame.board, newUnit)) {
        g_currentGame.unit = newUnit;
        g_currentGame.currentUnitHistory = [hashUnit(newUnit)];
      } else {
        showAlertMessage('CONFLICT!')
      }
    } else {
      showAlertMessage('GAME CLEAR!')
    }
  }

  if (g_currentGame.unit === undefined) {
    if (!g_currentGame.done) {
      done();
    }
  }

  var boardForDisplay = cloneBoard(g_currentGame.board);

  if (!dryUnit && g_currentGame.unit) {
    placeUnit(boardForDisplay, g_currentGame.unit, true, false, true);
  }

  if (dryUnit) {
    placeUnit(boardForDisplay, dryUnit, true, true);
  }

  var r = 12;
  var gridMul = 1.05;

  var boardMargin = {x: 10, y: 10};

  var boardCanvas = document.getElementById('board');
  boardContext = boardCanvas.getContext('2d');
  var boardGeometry = coordToPosition(
    {x: g_currentGame.configurations.width,
     y: g_currentGame.configurations.height}, r, gridMul);
  boardCanvas.width = boardGeometry.x + boardMargin.x * 2;
  boardCanvas.height = boardGeometry.y + boardMargin.y * 2;
  boardContext.clearRect(0, 0, boardCanvas.width, boardCanvas.height);
  drawBoard(boardForDisplay, r, gridMul, boardMargin, boardContext, true);

  var nextUnits = [];
  for (var i = 0; i < Math.min(g_currentGame.source.length, 20); ++i) {
    nextUnits.push(g_currentGame.configurations.units[g_currentGame.source[i]]);
  }

  var nextCanvas = document.getElementById('next');
  var nextContext = nextCanvas.getContext('2d');
  var nextMargin = {x: 20, y: 20};
  var nextBoundBox =
    getDrawUnitsBoundBox(nextUnits, r, gridMul, nextMargin);
  nextCanvas.width = nextBoundBox.width;
  nextCanvas.height = nextBoundBox.height;
  nextContext.clearRect(0, 0, nextCanvas.width, nextCanvas.height);
  drawUnits(nextUnits, r, gridMul, {x: 0, y: 0}, nextMargin, nextContext);

  var listCanvas = document.getElementById('list');
  var listContext = listCanvas.getContext('2d');
  var listMargin = {x: 20, y: 20};
  var listBoundBox =
    getDrawUnitsBoundBox(g_currentGame.configurations.units, r, gridMul, listMargin);
  listCanvas.width = listBoundBox.width;
  listCanvas.height = listBoundBox.height;
  listContext.clearRect(0, 0, listCanvas.width, listCanvas.height);
  drawUnits(g_currentGame.configurations.units, r, gridMul, {x: 0, y: 0}, listMargin,
            listContext);

  updateInfo();
}

function saveGame() {
  g_history.push(cloneGame(g_currentGame));
}

function done() {
  sendSolution();
}

function setupGame(configurations, seed) {
  var board = createBoard(configurations.width, configurations.height);
  var filled = configurations.filled;
  for (var i = 0; i < filled.length; ++i) {
    var filledCell = filled[i];
    board[filledCell.x][filledCell.y] = 1;
  }

  g_currentGame = {
    source: calcRandSeq(configurations.units.length,
                        seed,
                        configurations.sourceLength),
    board: board,
    configurations: configurations,
    unit: undefined,
    score: 0,
    ls_old: 0,
    commandHistory: '',
    done: false,
    seed: seed,
  };
  g_history = [];
  g_dryRun = false;
  saveGame();

  updateInfo();

  drawGame(undefined);
}

function drawProblem(file) {
  var x = new XMLHttpRequest();
  x.open('GET', '../problems/' + file);
  x.responseType = 'json';
  x.onload = function () {
    var seeds = document.getElementById('seeds');

    while (seeds.firstChild) {
      seeds.removeChild(seeds.firstChild);
    }

    var json = x.response;

    for (var i = 0; i < json.sourceSeeds.length; ++i) {
      var option = document.createElement('option');
      option.value = json.sourceSeeds[i];
      option.innerText = json.sourceSeeds[i];
      seeds.appendChild(option);
    }

    seeds.addEventListener('change', function () {
      var seed = parseInt(seeds.options[seeds.selectedIndex].value);
      console.log(seed);
      setupGame(json, seed);
    });

    setupGame(json, 0);
  };
  x.send();
}

function placeUnit(board, unit, placePivot, dryUnit, currentUnit) {
  var members = unit.members;

  for (var j = 0; j < members.length; ++j) {
    if (!inBoard(board, members[j])) {
      continue;
    }

    board[members[j].x][members[j].y] |= 1 << (dryUnit ? 2 : 0) << (currentUnit ? 4 : 0);
  }

  if (!placePivot) {
    return;
  }

  var pivot = unit.pivot;

  if (!inBoard(board, pivot)) {
    return;
  }

  board[pivot.x][pivot.y] |= 2 << (dryUnit ? 2 : 0) << (currentUnit ? 4 : 0);
}

function drawHex(center, r, context) {
  context.beginPath();
  for (var i = 0; i < 6; ++i) {
    var d = Math.PI / 6.0 + Math.PI * i / 3.0;
    context.lineTo(center.x + r * Math.cos(d), center.y + r * Math.sin(d));
  }
  context.closePath();
}

function coordToPosition(p, r, gridMul) {
  return {x: p.x * r * gridMul * 1.7 + r * Math.sqrt(3) / 2.0 * gridMul,
          y: p.y * r * gridMul * 1.5 + r * gridMul};
}

function logKey() {
  var log = document.getElementById('log');
  log.value = g_currentGame.commandHistory;
}

function sendSolution() {
  var str = JSON.stringify([{problemId: g_currentGame.configurations.id,
                             seed: g_currentGame.seed,
                             tag: 'handplay_viz',
                             solution: g_currentGame.commandHistory}]);
  console.log(str);

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

function drawBoard(board, r, gridMul, topLeft, context, showNumbers) {
  for (var x = 0; x < board.length; ++x) {
    context.strokeStyle = 'lightgray';
    context.fillStyle = 'lightgray';
    if (showNumbers) {
      var numPosition = coordToPosition({x: x, y: 0}, r, gridMul);
      context.fillText(x,
                       topLeft.x + numPosition.x - 5,
                       topLeft.y + numPosition.y + 5);
    }

    for (var y = 0; y < board[0].length; ++y) {
      var position = coordToPosition({x: x, y: y}, r, gridMul);
      var center = {x: position.x + topLeft.x, y: position.y + topLeft.y};
      if (y % 2 == 1) {
        center.x += r * Math.sqrt(3) / 2.0 * gridMul;
      }

      context.strokeStyle = 'lightgray';
      context.fillStyle = 'lightgray';
      if (showNumbers && x == 0) {
        context.fillText(y, center.x - 5, center.y + 5);
      }

      context.strokeStyle = 'black';
      context.fillStyle = 'black';

      drawHex(center, r, context);

      var data = board[x][y];
      if ((data & 1) == 1) {
        context.fill();
      } else {
        context.stroke();
      }

      if ((data & 4) == 4) {
        context.strokeStyle = 'red';
        context.fillStyle = 'red';
        drawHex(center, r * 0.7, context);
        context.fill();
      }

      if ((data & 16) == 16) {
        context.strokeStyle = 'black';
        context.fillStyle = 'black';
        drawHex(center, r * 0.7, context);
        context.fill();
      }

      if ((data & 32) == 32) {
        context.strokeStyle = 'gray';
        context.fillStyle = 'gray';
        drawHex(center, r * 0.4, context);
        context.fill();
      }

      if ((data & 8) == 8) {
        context.strokeStyle = 'pink';
        context.fillStyle = 'pink';
        drawHex(center, r * 0.4, context);
        context.fill();
      }
    }
  }
}
