const deviceIp = '192.168.4.1';
const poolIntervalMs = 100;
const startUrl = `http://${deviceIp}/s`;
const valueUrl = `http://${deviceIp}/v`;

const overCurrentValue = -1;
let overCurrent = false;
const queueMaxItems = 10;

let divError;

let queueArr = new Array();

const optionsChart = {
    width: 500,
    height: 300
};

function showError(textError) {
    divError.textContent = textError;
    divError.style = 'display:block';
}

function clearError() {
    divError.textContent = '';
    divError.style = 'display:none';
}

function GET(resource, func) {
    const xhr = new XMLHttpRequest();
    xhr.open('GET', resource, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            if (xhr.status != 200)
                throw `Get error:${resource} . HTTP-status:${xhr.status}`;
            func(xhr.response);
        }
    };
    xhr.send();
}

function devicePooling(response) {
    let interval = setInterval(() => {
        GET(valueUrl, handleValue);
        if (overCurrent) {
            clearInterval(interval);
            showError("Over current");
        }
    }, poolIntervalMs);
}

function start() {

    clearError();
    overCurrent = false;

    try {
        GET(startUrl, devicePooling);
    } catch (expcetion) {
        showError(expcetion);
    }

}

window.onload = () => {

    divError = document.getElementById("error");
    clearError();

    document.getElementById("buttonStart").onclick = start;
}

function addToQueue(value) {
    if (queueArr.length < queueMaxItems) {
        queueArr.push(value);
        return;
    }

    // Shift left one element
    for (let idx = 0; idx < queueMaxItems - 1; ++idx)
        queueArr[idx] = queueArr[idx + 1];
    queueArr[queueMaxItems - 1] = value;
}

function handleValue(value) {
    if (value == overCurrentValue) {
        overCurrent = true;
        return;
    }
    // Show chart
    addToQueue(value);

    new Chartist.Line('#chart', {
        labels: [],
        series: [queueArr]
    }/*, optionsChart*/);

}