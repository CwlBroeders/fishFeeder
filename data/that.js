function onBodyLoad() {
    var manualOverride = document.getElementById("manualOverride");
    manualOverride.onclick = Override;
    var newFeedTime = document.getElementById("newFeedTime");
    newFeedTime.onclick = setMoment;
    var cfgBtn = document.getElementById("cfgBtn");
    cfgBtn.onclick = configure;
    var confSub = document.getElementById("confSubmit");
    confSub.onclick = confSubmit;
    var readme = document.getElementById("readme");
    readme.onclick = instructVis;
    time = document.getElementById("time");
    loadValues();
    var run = setInterval(chronos, 1000)
}
var time, setFeedTimes, timeStamp, hrs, mins, secs;
var running = !1;
var maxFeedTime = 0;

function loadValues() {
    var xh = new XMLHttpRequest();
    xh.onreadystatechange = function() {
        if (xh.readyState == 4) {
            if (xh.status == 200) {
                var res = JSON.parse(xh.responseText);
                var eepConfSetsDur = res.duration;
                document.getElementById("duration").setAttribute('value', eepConfSetsDur);
                var eepConfSetsMot = res.motion;
                document.getElementById("angle").setAttribute('value', eepConfSetsMot);
                timeStamp = res.time.split(".");
                hrs = timeStamp[0];
                mins = timeStamp[1];
                secs = timeStamp[2];
                var setFeedTimes = res.feedTimes;
                var x = setFeedTimes.length;
                maxFeedTime = x;
                while (document.getElementById("feedTimeForm").hasChildNodes()) {
                    document.getElementById("feedTimeForm").removeChild(document.getElementById("feedTimeForm").lastChild)
                }
                for (var i = 0; i < x; i++) {
                    var newTimeOutput = document.createElement("div");
                    newTimeOutput.setAttribute('id', 'time' + i);
                    newTimeOutput.setAttribute('name', 'feedTime[]');
                    newTimeOutput.setAttribute('class', 'feedTimes');
                    newTimeOutput.setAttribute('data-value', setFeedTimes[i]);
                    newTimeOutput.innerHTML = setFeedTimes[i];
                    document.getElementById("feedTimeForm").appendChild(newTimeOutput);
                    newTimeOutput.onclick = editMoment
                }
                var elem = document.getElementById("instructions");
                if (x > 2) {
                    elem.style.display = 'none'
                } else {
                    elem.style.display = 'block'
                }
            }
        }
    };
    xh.open("POST", "/load", !0);
    xh.send(null)
};
var hideReadme = !1;

function instructVis() {
    var elem = document.getElementById("instructions");
    if (hideReadme == !0) {
        elem.style.display = 'none';
        hideReadme = !hideReadme
    } else {
        elem.style.display = 'block';
        hideReadme = !hideReadme
    }
}

function chronos() {
    secs++;
    if (secs > 59) {
        secs = 0;
        mins++
    }
    if (mins > 59) {
        mins = 0;
        hrs++
    }
    if (hrs > 23) {
        hrs = 0
    }
    currentHours = (hrs < 10 ? "0" : "") + hrs;
    currentMinutes = (mins < 10 ? "0" : "") + mins;
    time.innerHTML = currentHours + ":" + currentMinutes
}

function postFeedTimes() {
    var xh = new XMLHttpRequest();
    xh.open("POST", "/feedTime", !0);
    xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    var instructions = document.getElementById("instructions");
    var valsString = "";
    var feedTimeVals = document.getElementsByClassName("feedTimes");
    var fTvallngth = feedTimeVals.length;
    for (var i = 0; i < fTvallngth; i++) {
        if (feedTimeVals[i].dataset.value !== '') {
            valsString += feedTimeVals[i].getAttribute("name");
            valsString += '=';
            valsString += feedTimeVals[i].dataset.value;
            valsString += "&"
        }
    }
    if (valsString == "") {
        valsString += "feedTime[]=--:--&"
    }
    valsString = valsString.substring(0, valsString.length - 1);
    xh.send(valsString);
    loadValues()
};

function Override() {
    var confvals = document.getElementsByClassName("conf");
    if (confvals[0].value !== '' && confvals[1].value !== '') {
        var xh = new XMLHttpRequest();
        xh.open("POST", "/Override", !0);
        xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        var valsString = "Override=true";
        xh.send(valsString)
    } else {
        configure()
    }
};

function editMoment() {
    maxFeedTime--;
    document.getElementById("inputMoment").value = this.dataset.value;
    var addBtn = document.getElementById("newFeedTime");
    if (addBtn.value == 'Add' || addBtn.value == 'maxed') {
        addBtn.setAttribute('value', 'Edit')
    }
    this.parentNode.removeChild(this)
}

function setMoment() {
    var newMoment = document.getElementById("inputMoment").value;
    var elem = document.getElementById("instructions");
    if (maxFeedTime < 1) {
        elem.style.display = 'block'
    } else {
        elem.style.display = 'none'
    }
    if (maxFeedTime < 8) {
        maxFeedTime++;
        var newTimeInput = document.createElement("div");
        newTimeInput.setAttribute('name', 'feedTime[]');
        newTimeInput.setAttribute('class', 'feedTimes');
        newTimeInput.setAttribute('data-value', newMoment);
        newTimeInput.innerHTML = newMoment;
        document.getElementById("feedTimeForm").appendChild(newTimeInput);
        newTimeInput.onclick = editMoment;
        document.getElementById("newFeedTime").setAttribute('value', 'Add')
    } else {
        document.getElementById("newFeedTime").setAttribute('value', 'maxed')
    }
    postFeedTimes()
}
var hidecConfigPopup = !1;

function configure() {
    configPopup = document.getElementById("configPopup");
    if (hidecConfigPopup == !0) {
        configPopup.style.display = 'none';
        hidecConfigPopup = !1
    } else {
        configPopup.style.display = 'block';
        hidecConfigPopup = !0
    }
}

function confSubmit() {
    var xh = new XMLHttpRequest();
    xh.open("POST", "/conf", !0);
    xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    var valsString = "";
    var confvals = document.getElementsByClassName("conf");
    var vallngth = confvals.length;
    for (var i = 0; i < vallngth; i++) {
        if (confvals[i].value !== '') {
            valsString += confvals[i].getAttribute("name");
            valsString += '=';
            valsString += confvals[i].value;
            valsString += "&"
        }
    }
    if (valsString == "") {
        xh.abort()
    }
    console.log(valsString);
    valsString = valsString.substring(0, valsString.length - 1);
    xh.send(valsString);
    configure()
}
