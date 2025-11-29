const cards = Array.from(document.querySelectorAll('.question-card'));
let idx = 0, score = 0, maxScore = 0, qInterval = null;
const lockedIds = new Set(jumps.map(j => j.target));

if (shouldShuffle) {
    for (let i = cards.length - 1; i > 0; i--) {
        if (!lockedIds.has(cards[i].id)) {
            let j = Math.floor(Math.random() * (i + 1));
            while (j > 0 && lockedIds.has(cards[j].id)) j--;
            if (j >= 0 && !lockedIds.has(cards[j].id)) {
                [cards[i], cards[j]] = [cards[j], cards[i]];
            }
        }
    }
}

lockedIds.forEach(id => {
    let el = document.getElementById(id);
    if (el) {
        let b = document.createElement('div');
        b.innerText = '★ Pregunta Bonus';
        b.style.cssText = 'color:#f59e0b;font-weight:bold;margin-bottom:10px';
        el.querySelector('.q-header').before(b);
    }
});

function shuffleOptions(card) {
    if (card.dataset.shuffleOpts === 'true') {
        let optionsContainer = card.querySelector('.options-container');
        
        if (optionsContainer) {
            let labels = Array.from(optionsContainer.querySelectorAll('.opt-label'));
            for (let i = labels.length - 1; i > 0; i--) {
                const j = Math.floor(Math.random() * (i + 1));
                optionsContainer.appendChild(labels[j]);
            }
        }
    }
}

if (cards.length > 0) {
    shuffleOptions(cards[0]);
    cards[0].classList.add('active');
    startQTimer(cards[0]);
}

const timerEl = document.getElementById('timer');
let time = timerEl ? parseInt(timerEl.dataset.sec) : 0;
let interval;

if (time > 0) {
    timerEl.style.display = 'block';
    updateTimerDisplay();
    interval = setInterval(() => {
        time--;
        updateTimerDisplay();
        if (time <= 0) {
            clearInterval(interval);
            finish(true);
        }
    }, 1000);
}

function updateTimerDisplay() {
    let m = Math.floor(time / 60), s = time % 60;
    timerEl.innerText = `${m}:${s < 10 ? '0' : ''}${s}`;
    if (time <= 10) timerEl.classList.add('danger');
}

function startQTimer(c) {
    if (!c) return;
    let t = parseInt(c.dataset.qtime) || 0;
    if (t <= 0) return;
    let cur = t, d = c.querySelector('.q-val');
    if (d) d.innerText = cur;
    qInterval = setInterval(() => {
        cur--;
        if (d) d.innerText = cur;
        if (cur <= 0) {
            clearInterval(qInterval);
            c.querySelector('.btn-next')?.click();
        }
    }, 1000);
}

function evalCondition(op, left, right) {
    switch(op) {
        case '==': return left == right;
        case '!=': return left != right;
        case '>':  return left > right;
        case '<':  return left < right;
        case '>=': return left >= right;
        case '<=': return left <= right;
        default:   return false;
    }
}

function next(btn) {
    if (qInterval) {
        clearInterval(qInterval);
        qInterval = null;
    }
    
    let card = btn.closest('.question-card');
    if (card.dataset.processed) return;
    card.dataset.processed = 'true';
    
    let pts = parseFloat(card.dataset.p) || 0;
    let pWrong = parseFloat(document.getElementById('p-wrong').value);
    let ans = card.dataset.ans;
    let type = card.dataset.type;
    let caseSens = card.dataset.case === 'true';
    let isBonus = lockedIds.has(card.id);
    
    let user = '', correct = false;
    let partialCredit = card.dataset.partial === 'true';
    let isMulti = card.dataset.multi === 'true';
    let earnedPoints = 0;
    
    if (isMulti) {
        let checked = Array.from(card.querySelectorAll('input:checked')).map(e => e.value);
        user = checked.join(',');
    } else if (type.includes('multiple') || type.includes('true')) {
        let el = card.querySelector('input:checked');
        if (el) user = el.value;
    } else {
        let el = card.querySelector('input[type=text]');
        if (el) user = el.value.trim();
    }
    
    if (user) {
        if (isMulti && partialCredit) {
            let correctAnswers = ans.split(',').map(a => a.trim().toLowerCase());
            let userAnswers = user.split(',').map(a => a.trim().toLowerCase());
            
            let correctCount = userAnswers.filter(u => correctAnswers.includes(u)).length;
            let wrongCount = userAnswers.filter(u => !correctAnswers.includes(u)).length;
            
            if (wrongCount === 0 && correctCount > 0) {
                earnedPoints = (correctCount / correctAnswers.length) * pts;
                correct = (correctCount === correctAnswers.length); 
                card.dataset.result = correct ? 'correct' : 'partial';
            } else {
                earnedPoints = 0;
                correct = false;
                card.dataset.result = 'wrong';
            }
        } else if (isMulti) {
            let correctAnswers = ans.split(',').map(a => a.trim().toLowerCase()).sort();
            let userAnswers = user.split(',').map(a => a.trim().toLowerCase()).sort();
            correct = JSON.stringify(correctAnswers) === JSON.stringify(userAnswers);
            card.dataset.result = correct ? 'correct' : 'wrong';
            earnedPoints = correct ? pts : 0;
        } else {
            if (caseSens) {
                correct = (user === ans);
            } else {
                correct = (user.toLowerCase() === ans.toLowerCase());
            }
            if (!correct && ans.includes(',')) {
                if (ans.split(',').some(a => a.trim().toLowerCase() === user.toLowerCase())) {
                    correct = true;
                }
            }
            card.dataset.result = correct ? 'correct' : 'wrong';
            earnedPoints = correct ? pts : 0;
        }
    }
    
    if (correct || (partialCredit && earnedPoints > 0)) {
        score += earnedPoints;
        if (!isBonus) maxScore += pts;
    } else {
        if (!isBonus) {
            score += pWrong;
            maxScore += pts;
        }
    }
    
    card.classList.remove('active');
    
    let tid = null;
    for (let j of jumps) {
        let ifTarget = document.getElementById(j.target);
        let elseTarget = j.elseTarget ? document.getElementById(j.elseTarget) : null;
        
        let ifNextIndex = ifTarget ? cards.indexOf(ifTarget) : -1;
        let elseNextIndex = elseTarget ? cards.indexOf(elseTarget) : -1;
        
        if ((ifNextIndex === idx + 1) || (elseNextIndex === idx + 1)) {
            // el l-value!
            let leftValue;
            if (j.variable === 'score') {
                leftValue = score;
            } else if (j.variable === 'timeLeft') {
                leftValue = time;
            } else if (j.variable === 'points') {
                leftValue = pts;
            } else {
                leftValue = score; // default
            }
            
            if (evalCondition(j.op, leftValue, j.threshold)) {
                tid = j.target;
            } else if (j.elseTarget) {
                tid = j.elseTarget;
            }
        }
    }
    
    let nextCard = null;
    if (tid) {
        nextCard = document.getElementById(tid);
        if (nextCard) idx = cards.indexOf(nextCard);
    }
    
    if (!nextCard) {
        let n = idx + 1;
        while (n < cards.length && lockedIds.has(cards[n].id)) n++;
        if (n < cards.length) {
            idx = n;
            nextCard = cards[idx];
        }
    }
    
    if (nextCard) {
        shuffleOptions(nextCard);
        nextCard.classList.add('active');
        startQTimer(nextCard);
    } else {
        finish(false);
    }
}

function finish(to) {
    clearInterval(interval);
    if (qInterval) clearInterval(qInterval);
    
    document.querySelector('h1').style.display = 'none';
    if (timerEl) timerEl.style.display = 'none';
    
    let res = document.getElementById('result');
    let pass = maxScore > 0 ? (score / maxScore >= 0.5) : false;
    
    res.className = 'result-box ' + (pass ? 'pass' : 'fail');
    res.style.display = 'flex';
    
    res.innerHTML = (to ? '<h3>¡Tiempo Agotado!</h3>' : '') +
        (pass ? '<h1>¡Felicidades!</h1>' : '<h1>Inténtalo de Nuevo</h1>') +
        `<div class='score-num'>${score.toFixed(1)} / ${maxScore.toFixed(1)}</div>` +
        '<p>Puntaje Final</p>' +
        '<h3 style="margin-top:30px;border-top:1px solid #ccc;padding-top:20px;">Revisión</h3>';
    
    let cont = document.querySelector('.quiz-container');
    cont.style.overflowY = 'auto';
    cont.style.height = 'auto';
    cont.insertBefore(res, cont.firstChild);
    
    cards.forEach(c => {
        c.style.display = 'block';
        c.classList.remove('active');
        c.style.marginBottom = '20px';
        c.style.pointerEvents = 'none';
        c.style.opacity = '1';
        c.style.animation = 'none';
        
        if (c.dataset.result === 'correct') {
            c.style.border = '3px solid #10b981';
            c.style.background = '#ecfdf5';
        } else if (c.dataset.result === 'partial') {
            c.style.border = '3px solid #f59e0b';
            c.style.background = '#fffbeb';
        } else {
            c.style.border = '3px solid #ef4444';
            c.style.background = '#fef2f2';
        }
        
        c.querySelector('.btn-next').style.display = 'none';
    });
}