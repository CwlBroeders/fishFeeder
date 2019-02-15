
//  onBodyLoad runs after the DOM has finished loading
//  all of the DOM elements we need to manipulate can now be assigned
//  this makes sure all the elements actually excist when we assign them
//  onBodyLoad is called from <script> in the HTML <head>.
function onBodyLoad() {
    var manualOverride = document.getElementById("manualOverride");             //  the manualOverride button
    manualOverride.onclick = Override;
    var newFeedTime = document.getElementById("newFeedTime");                   //  input for the feed times, call setMoment() when clicked
    newFeedTime.onclick = setMoment;
    var cfgBtn = document.getElementById("cfgBtn");                             //  config button to make the inputs visible
    cfgBtn.onclick = configure;
    var confSub = document.getElementById("confSubmit");                        //  button to submit the servo configuration
    confSub.onclick = confSubmit;
    var readme = document.getElementById("readme");                             //  button to toggle the user instructions
    readme.onclick = instructVis;
    time = document.getElementById("time");                                     //  the element that will hold the clock
    loadValues();
    var run = setInterval(chronos, 1000)
}
var time, setFeedTimes, timeStamp, hrs, mins, secs;                             //  declare the variables and, if needed, assign values
var running = !1;
var maxFeedTime = 0;

//  loadValues calls our ESP and parses the JSON response
//  the JSON response will contain all the relevant data that may be stored in EEPROM
//  for more information on the XMLHttpRequest API visit
//  https://developer.mozilla.org/nl/docs/Web/API/XMLHttpRequest
function loadValues() {
    var xh = new XMLHttpRequest();
    xh.onreadystatechange = function() {
        if (xh.readyState == 4) {                                               //  4	DONE	The operation is complete.
            if (xh.status == 200) {                                             //  https://httpstatuses.com/200
                var res = JSON.parse(xh.responseText);                          //  parse the returned JSON and store data to the result variable
                var eepConfSetsDur = res.duration;                              //  pass value from sesponse to DOM element
                document.getElementById("duration").setAttribute('value', eepConfSetsDur);
                var eepConfSetsMot = res.motion;
                document.getElementById("angle").setAttribute('value', eepConfSetsMot);
                timeStamp = res.time.split(".");                                //  seperate the returned value into hours minute and second and pass to variables for tiime keeping
                hrs = timeStamp[0];
                mins = timeStamp[1];
                secs = timeStamp[2];
                var setFeedTimes = res.feedTimes;
                var x = setFeedTimes.length;
                maxFeedTime = x;
                 //  if feedtimes were removed, we would have too many elements, so we remove them and create the amount we need
                while (document.getElementById("feedTimeForm").hasChildNodes()) {
                    document.getElementById("feedTimeForm").removeChild(document.getElementById("feedTimeForm").lastChild)
                }
                //  now loop thru all of the times we had returned and create a new new <div> element and give it the correct attributes
                for (var i = 0; i < x; i++) {
                    var newTimeOutput = document.createElement("div");
                    newTimeOutput.setAttribute('id', 'time' + i);
                    newTimeOutput.setAttribute('name', 'feedTime[]');           //  note that the 'name' attribute has squared brackets, for PHP this would indicate an array
                    newTimeOutput.setAttribute('class', 'feedTimes');
                    newTimeOutput.setAttribute('data-value', setFeedTimes[i]);  //  this is the value that will be passed down with POST
                    newTimeOutput.innerHTML = setFeedTimes[i];
                    document.getElementById("feedTimeForm").appendChild(newTimeOutput);
                    newTimeOutput.onclick = editMoment                          //  give the element a callback function on a click event, this will allow our user to edit our feedtimes later
                }
                var elem = document.getElementById("instructions");             //  check if the instructions should be displayed, we will show instructions by default if there are 2 or less feedtimes
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
//  if the readme button is clicked we must display the instructions
function instructVis() {
    var elem = document.getElementById("instructions");
    if (hideReadme == !0) {
        elem.style.display = 'none';                                            // set this elements CSS to display:none;
        hideReadme = !hideReadme
    } else {
        elem.style.display = 'block';                                           // set this elements CSS to display:block;
        hideReadme = !hideReadme
    }
}
//  clock function to keep current with the clock on our ESP
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
    time.innerHTML = currentHours + ":" + currentMinutes                        //  update the the text in the time <div> to show the current time
}
//  use the POST method to send our feedTimes to our ESP
function postFeedTimes() {
    var xh = new XMLHttpRequest();
    xh.open("POST", "/feedTime", !0);
    xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    var valsString = "";
    var feedTimeVals = document.getElementsByClassName("feedTimes");            //  collect all the elements of class "feedTimes"
    var fTvallngth = feedTimeVals.length;
    for (var i = 0; i < fTvallngth; i++) {
        if (feedTimeVals[i].dataset.value !== '') {                             //  check if any value has actually been set, if so, append it to our payload
            valsString += feedTimeVals[i].getAttribute("name");
            valsString += '=';
            valsString += feedTimeVals[i].dataset.value;
            valsString += "&"                                                   //  add & after the value so we can reasily repead this for multiple values
        }
    }
    if (valsString == "") {                                                     //  if there are no feedtimes, send --:-- as a value, our sketch will check for this
        valsString += "feedTime[]=--:--&"                                       //  need to add the & at the end to stay consistent
    }
    valsString = valsString.substring(0, valsString.length - 1);                //  remove the & at the end of the last value
    console.log(valsString);                                                    //  print the value in the browser console
    xh.send(valsString);
    loadValues()                                                                //  reload values as to check
};

//  tell the ESP to trigger the servo when the override button is pushed
function Override() {
    var confvals = document.getElementsByClassName("conf");
    if (confvals[0].value !== '' && confvals[1].value !== '') {                 //  check if servo movement and duration variables have been set
        var xh = new XMLHttpRequest();
        xh.open("POST", "/Override", !0);
        xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        var valsString = "Override=true";
        xh.send(valsString)
    } else {                                                                    //    if not, ask the user to configure them
        configure()
    }
};

//  the user clicked a feedtime to edit/remove
function editMoment() {
    maxFeedTime--;
    document.getElementById("inputMoment").value = this.dataset.value;
    var addBtn = document.getElementById("newFeedTime");
    if (addBtn.value == 'Add' || addBtn.value == 'maxed') {
        addBtn.setAttribute('value', 'Edit')
    }
    this.parentNode.removeChild(this)
}

// the user have a new time value he wants to set
function setMoment() {
    var newMoment = document.getElementById("inputMoment").value;
    var elem = document.getElementById("instructions");                         //  check if the instructions still need to be shown
    if (maxFeedTime < 1) {
        elem.style.display = 'block'
    } else {
        elem.style.display = 'none'
    }
    if (maxFeedTime < 8) {                                                      //   check if we dont have 8 values already
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
    postFeedTimes()                                                             //  call the postFeedTimes() function to update the ESP
}

//  check if the configuration menu needs to be shown
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

//  POST the configuration values for the servo to the ESP
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
    if (valsString == "") {                                                     //  check if a value has actually been set, abort if not
        xh.abort()
    }
    console.log(valsString);                                                    //  print the value in the browser console
    valsString = valsString.substring(0, valsString.length - 1);
    xh.send(valsString);
    configure()                                                                 //  servo has been configured so we can close the config menu
}
