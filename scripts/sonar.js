const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
});
let indata = []
rl.on('line', (line) => indata.push(line))
rl.once('close', () => convert(JSON.parse(indata.join('\n'))))

function convert(data) {
    console.log(JSON.stringify({
        version: '14.0.4', vulnerabilities: data.issues.map(data => ({
            id: data.key || data.hash,
            category: 'sast',
            message: data.message,
            cve: data.rule,
            scanner: {
                id: 'sonarqube',
                name: 'sonarqube'
            },
            description: '',
            location: { file: data.component.substring(data.component.indexOf(':') + 1), line: data.line || data.textRange.startLine },
            confidence: 'High',
            severity: levels[data.severity || 'MAJOR'],
            identifiers: [{
                "type": 'sonar ' + data.type.toLowerCase(),
                "name": "sonar warning",
                "value": "sonar"
            }],
            links: [{
                name: 'sonarqube',
                url: `https://sonarqube.bc-labs.dev/project/issues?issues=${data.key}&open=${data.key}&id=${data.project}`
            }]
        }))
    }, null, 2))
}
const levels = {

    INFO: 'Info',
    MINOR: 'Low',
    MAJOR: 'Medium',
    CRITICAL: 'High',
    BLOCKER: 'Critical'
}





























/*




const https = require('https');
const fs = require('fs')
const project = 'sdk-core'
const severities = 'BLOCKER,CRITICAL'
const branch = 'develop'
const url = `${(process.env.SONAR_HOST_URL || '')}/api/issues/search?componentKeys=${project}&types=BUG,VULNERABILITY&branch=${branch}&severities=${severities}`
const token = process.env.SONAR_TOKEN || 'sqp_49f5f64dde7a2cfe2edc81944b462b1d734db750'




https.get('https://api.nasa.gov/planetary/apod?api_key=DEMO_KEY', (resp) => {
    let data = '';

    // A chunk of data has been received.
    resp.on('data', (chunk) => {
        data += chunk;
    });

    // The whole response has been received. Print out the result.
    resp.on('end', () => {
        console.log(JSON.parse(data).explanation);
    });

}).on("error", (err) => {
    console.log("Error: " + err.message);
});
*/