var g_canvasContext;

var g_currentGame;
var g_history;

function determineStart(board, unit) {
  var unitBoundBox = getUnitBoundBox(unit, false);

  var unitWidth = unitBoundBox.right - unitBoundBox.left + 1;
  if (unitWidth % 2 == 0 && board.length % 2 == 0) {
    return {x: board.length / 2 - unitWidth / 2, y: -unitBoundBox.top};
  } else if (unitWidth % 2 == 0) {
    return {x: (board.length - unitWidth - 1) / 2, y: -unitBoundBox.top};
  } else if (board.length % 2 == 0) {
    return {x: (board.length - unitWidth - 1) / 2, y: -unitBoundBox.top};
  } else {
    return {x: (board.length - unitWidth) / 2, y: -unitBoundBox.top};
  }
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
  var xx = position.x - Math.floor(position.y / 2);
  var zz = position.y;
  var yy = -xx - zz;

  var tmp = xx;
  xx = -zz;
  zz = -yy;
  yy = -tmp;
  position.x = xx + Math.floor(zz / 2);
  position.y = zz;
}

function rotateCounterClockwise(position) {
  var xx = position.x - Math.floor(position.y / 2);
  var zz = position.y;
  var yy = -xx - zz;

  var tmp = xx;
  xx = -yy;
  yy = -zz;
  zz = -tmp;
  position.x = xx + Math.floor(zz / 2);
  position.y = zz;
}

function moveClockwise(origin, position) {
  var moved = {x: position.x - origin.x,
               y: position.y - origin.y};

  if ((origin.y & 1) && !(position.y & 1)) {
    moved.x -= 1;
  }

  rotateClockwise(moved);

  moved.x += origin.x;
  moved.y += origin.y;

  if ((origin.y & 1) && (!(moved.y & 1))) {
    moved.x += 1;
  }

  position.x = moved.x;
  position.y = moved.y;
}

function moveCounterClockwise(origin, position) {
  var moved = {x: position.x - origin.x,
               y: position.y - origin.y};

  if ((origin.y & 1) && !(position.y & 1)) {
    moved.x -= 1;
  }

  rotateCounterClockwise(moved);

  if ((origin.y & 1) && ((moved.y & 1))) {
    moved.x += 1;
  }

  moved.x += origin.x;
  moved.y += origin.y;

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

function drawUnit(unit, topLeft, r, gridMul) {
  var unitBoundBox = getUnitBoundBox(unit, true);
  var board = createBoard(unitBoundBox.right + 1, unitBoundBox.bottom + 1);
  placeUnit(board, unit, true);
  drawBoard(board, r, gridMul, topLeft);
  return coordToPosition({x: 0, y: unitBoundBox.bottom + 1}, r, gridMul).y;
}

function drawUnits(units, r, gridMul, topLeft) {
  var margin = {x: 20, y: 20};

  var totalWidth = 0;
  var totalHeight = 0;

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

  var top = margin.x;
  var left = margin.y;

  g_canvasContext.fillText('Units coming', topLeft.x + left, topLeft.y + top + 10);
  top += 10;

  for (var i = 0; i < Math.min(g_currentGame.source.length, 5); ++i) {
    top += drawUnit(units[g_currentGame.source[i]],
                    {x: topLeft.x + left, y: topLeft.y + top}, r, gridMul);
  }

  g_canvasContext.fillText('=============', topLeft.x + left, topLeft.y + top + 10);
  top += 10;
  g_canvasContext.fillText('All units', topLeft.x + left, topLeft.y + top + 10);
  top += 10;

  for (var i = 0; i < units.length; ++i) {
    var unit = units[i];

    g_canvasContext.fillText(i, topLeft.x + left, topLeft.y + top + 10);
    top += 10;

    var unitBoundBox = getUnitBoundBox(unit, true);
    var board = createBoard(unitBoundBox.right + 1, unitBoundBox.bottom + 1);
    placeUnit(board, unit, true);

    drawBoard(board, r, gridMul, {x: topLeft.x + left, y: topLeft.y + top});

    top += coordToPosition({x: 0, y: unitBoundBox.bottom + 1}, r, gridMul).y;
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
  updateScore();

  g_currentGame.board = result.board;
}

function updateScore() {
  if (g_currentGame === undefined) {
    return;
  }

  var scoreDiv = document.getElementById("score");
  scoreDiv.innerText = g_currentGame.score + ' pts';
}

function undo() {
  if (g_history.length > 0) {
    g_currentGame = g_history.pop();
  }
  updateScore();
}

function handleKey(keyCode) {
  if (keyCode == 'U'.charCodeAt(0)) {
    undo();

    drawGame();
    logKey();
    return;
  }

  if (g_currentGame === undefined ||
      g_currentGame.unit === undefined) {
    return;
  }

  var newUnit = cloneUnit(g_currentGame.unit);

  var command;

  switch (keyCode) {
  case 'W'.charCodeAt(0):
    moveUnit(newUnit, moveCounterClockwise.bind(undefined, newUnit.pivot));
    command = 'k';
    break;

  case 'E'.charCodeAt(0):
    moveUnit(newUnit, moveClockwise.bind(undefined, newUnit.pivot));
    command = 'd';
    break;

  case 'A'.charCodeAt(0):
    moveUnit(newUnit, moveW);
    command = 'p';
    break;

  case 'Z'.charCodeAt(0):
    moveUnit(newUnit, moveSW);
    command = 'a';
    break;

  case 'X'.charCodeAt(0):
    moveUnit(newUnit, moveSE);
    command = 'l';
    break;

  case 'D'.charCodeAt(0):
    moveUnit(newUnit, moveE);
    command = 'b';
    break;

  default:
    return;
  }

  saveGame();
  g_currentGame.commandHistory += command;
  logKey();
  if (!isInvalidUnitPlacement(g_currentGame.board, newUnit)) {
    g_currentGame.unit = newUnit;
  } else {
    placeUnit(g_currentGame.board, g_currentGame.unit, false);
    doLock();
    g_currentGame.unit = undefined;
  }
  drawGame();
}

function init() {
  var canvas = document.getElementById('canvassample');
  g_canvasContext = canvas.getContext('2d');

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
    drawProblem(problems.options[problems.selectedIndex].value);
  });

  drawProblem(problems.options[problems.selectedIndex].value);

  document.body.addEventListener('keydown', function (e) {
    handleKey(e.keyCode);
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

function drawGame() {
  g_canvasContext.clearRect(0, 0, 4000, 4000);

  var r = 15;
  var gridMul = 1.1;

  if (g_currentGame.unit === undefined) {
    if (g_currentGame.source.length > 0) {
      var newUnit = cloneUnit(g_currentGame.configurations.units[g_currentGame.source.shift()]);
      var move = determineStart(g_currentGame.board, newUnit);
      moveUnit(newUnit, function (position) {
        position.x += move.x;
        position.y += move.y;
      });

      if (!isInvalidUnitPlacement(g_currentGame.board, newUnit)) {
        g_currentGame.unit = newUnit;
      }
    }
  }

  if (g_currentGame.unit === undefined) {
    if (!g_currentGame.done) {
      done();
    }
  }

  var boardForDisplay = cloneBoard(g_currentGame.board);

  if (g_currentGame.unit) {
    placeUnit(boardForDisplay, g_currentGame.unit, true);
  }

  var margin = {x: 10, y: 10};
  var position = margin;

  drawBoard(boardForDisplay, r, gridMul, position);

  position.x += coordToPosition(
    {x: g_currentGame.configurations.width, y: 0}, r, gridMul).x + 20;
  drawUnits(g_currentGame.configurations.units, r, gridMul, position);
}

function saveGame() {
  g_history.push(cloneGame(g_currentGame));
}

function done() {
  sendSolution();
}

function setupGame(configurations) {
  var board = createBoard(configurations.width, configurations.height);
  var filled = configurations.filled;
  for (var i = 0; i < filled.length; ++i) {
    var filledCell = filled[i];
    board[filledCell.x][filledCell.y] = 1;
  }

  g_currentGame = {
    source: calcRandSeq(configurations.units.length,
                        configurations.sourceSeeds[0],
                        configurations.sourceLength),
    board: board,
    configurations: configurations,
    unit: undefined,
    score: 0,
    ls_old: 0,
    commandHistory: '',
    done: false,
  };
  g_history = [];
  saveGame();

  updateScore();

  drawGame();
}

function drawProblem(file) {
  var x = new XMLHttpRequest();
  x.open('GET', '../problems/' + file);
  x.responseType = 'json';
  x.onload = function () {
    setupGame(x.response);
  };
  x.send();
}

function placeUnit(board, unit, placePivot) {
  var members = unit.members;

  for (var j = 0; j < members.length; ++j) {
    if (!inBoard(board, members[j])) {
      continue;
    }

    board[members[j].x][members[j].y] |= 1;
  }

  if (!placePivot) {
    return;
  }

  var pivot = unit.pivot;

  if (!inBoard(board, pivot)) {
    return;
  }

  board[pivot.x][pivot.y] |= 2;
}

function drawHex(center, r) {
  g_canvasContext.beginPath();
  for (var i = 0; i < 6; ++i) {
    var d = Math.PI / 6.0 + Math.PI * i / 3.0;
    g_canvasContext.lineTo(center.x + r * Math.cos(d), center.y + r * Math.sin(d));
  }
  g_canvasContext.closePath();
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
                             seed: g_currentGame.configurations.sourceSeeds[0],
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

function drawBoard(board, r, gridMul, topLeft) {
  for (var x = 0; x < board.length; ++x) {
    for (var y = 0; y < board[0].length; ++y) {
      var position = coordToPosition({x: x, y: y}, r, gridMul);
      var center = {x: position.x + topLeft.x, y: position.y + topLeft.y};
      if (y % 2 == 1) {
        center.x += r * Math.sqrt(3) / 2.0 * gridMul;
      }

      g_canvasContext.strokeStyle = 'black';
      g_canvasContext.fillStyle = 'black';

      drawHex(center, r);

      var data = board[x][y];
      if ((data & 1) == 1) {
        g_canvasContext.fill();
      } else {
        g_canvasContext.stroke();
      }

      if ((data & 2) == 2) {
        g_canvasContext.strokeStyle = 'gray';
        g_canvasContext.fillStyle = 'gray';
        drawHex(center, r * 0.4);
        g_canvasContext.fill();
      }
    }
  }
}
