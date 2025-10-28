#include <WiFi.h>
#include "config/config_manager.h"

using namespace std;

String mainHtmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
<title>Configure GlucoView</title>
<style>
    :root {
        --bg-light: #f3f4f6;
        --bg-dark: #111827;
        --card-light: #ffffff;
        --card-dark: #1f2937;
        --text-light: #1f2937;
        --text-dark: #f9fafb;
        --border-light: #d1d5db;
        --border-dark: #374151;
        --primary-light: #2563eb;
        --primary-dark: #3b82f6;
    }

    body {
        font-family: sans-serif;
        background: var(--bg-light);
        color: var(--text-light);
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
        padding: 1rem;
        transition: background 0.3s, color 0.3s;
    }

    body.dark {
        background: var(--bg-dark);
        color: var(--text-dark);
    }

    .form-card {
        background: var(--card-light);
        padding: 1.5rem;
        border-radius: 1rem;
        max-width: 500px;
        width: 100%;
        box-shadow: 0 4px 20px rgba(0,0,0,0.1);
        transition: background 0.3s;
        position: relative;
    }

    body.dark .form-card {
        background: var(--card-dark);
    }

    h1 {
        font-size: 1.5rem;
        margin-bottom: 1rem;
    }

    .section {
        margin-bottom: 1rem;
        border: 1px solid var(--border-light);
        border-radius: 0.5rem;
        overflow: hidden;
        transition: border-color 0.3s;
    }

    body.dark .section {
        border-color: var(--border-dark);
    }

    .section-header {
        padding: 0.75rem 1rem;
        font-weight: bold;
        background: #f9fafb;
        cursor: pointer;
        display: flex;
        justify-content: space-between;
        align-items: center;
        transition: background 0.3s;
    }

    body.dark .section-header {
        background: #374151;
    }

    .section-header:hover {
        background: #e5e7eb;
    }

    body.dark .section-header:hover {
        background: #4b5563;
    }

    .section-content {
        padding: 0 1rem;
        max-height: 0;
        overflow: hidden;
        transition: max-height 0.4s ease, padding 0.3s ease;
    }

    .section-content.open {
        padding: 1rem;
    }

    input[type="text"], input[type="password"] {
        width: 100%;
        padding: 0.6rem 0.75rem;
        margin-bottom: 0.75rem;
        border: 1px solid var(--border-light);
        border-radius: 0.5rem;
        font-size: 1rem;
        transition: background 0.3s, border-color 0.3s, color 0.3s;
        box-sizing: border-box;
    }

    body.dark input[type="text"], 
    body.dark input[type="password"] {
        background: var(--border-dark);
        border-color: var(--border-dark);
        color: var(--text-dark);
    }

    .switch {
        display: flex;
        justify-content: space-between;
        align-items: center;
        background: #f9fafb;
        padding: 0.5rem 0.75rem;
        border-radius: 0.5rem;
        margin-bottom: 0.5rem;
        transition: background 0.3s;
    }

    body.dark .switch {
        background: #374151;
    }

    .switch input {
        display: none;
    }

    .slider {
        position: relative;
        width: 44px;
        height: 24px;
        background: #d1d5db;
        border-radius: 9999px;
        transition: background 0.3s;
        cursor: pointer;
    }

    body.dark .slider {
        background: #4b5563;
    }

    select {
        width: 100%;
        padding: 0.6rem 0.75rem;
        margin-bottom: 0.75rem;
        border: 1px solid var(--border-light);
        border-radius: 0.5rem;
        font-size: 1rem;
        background: var(--card-light);
        color: var(--text-light);
        box-sizing: border-box;
        transition: background 0.3s, border-color 0.3s, color 0.3s;
    }

    body.dark select {
        background: var(--border-dark);
        border-color: var(--border-dark);
        color: var(--text-dark);
    }


    .slider::before {
        content: "";
        position: absolute;
        top: 3px;
        left: 3px;
        width: 18px;
        height: 18px;
        background: white;
        border-radius: 50%;
        transition: transform 0.3s;
    }

    input:checked + .slider {
        background: var(--primary-light);
    }

    body.dark input:checked + .slider {
        background: var(--primary-dark);
    }

    input:checked + .slider::before {
        transform: translateX(20px);
    }

    button.submit {
        width: 100%;
        padding: 0.75rem;
        background: var(--primary-light);
        border: none;
        color: white;
        border-radius: 0.5rem;
        font-size: 1rem;
        cursor: pointer;
        transition: background 0.3s;
    }

    button.submit:hover {
        background: #1d4ed8;
    }

    body.dark button.submit {
        background: var(--primary-dark);
    }

    body.dark button.submit:hover {
        background: #2563eb;
    }

    .theme-toggle {
        position: absolute;
        top: -3.5rem;
        right: 0;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        background: #e5e7eb;
        border: none;
        padding: 0.5rem;
        border-radius: 0.5rem;
        cursor: pointer;
        transition: background 0.3s;
        width: auto;
    }

    body.dark .theme-toggle {
        background: #4b5563;
        color: white;
    }

    .arrow {
        transition: transform 0.3s;
    }
    .arrow.open {
        transform: rotate(90deg);
    }
</style>
</head>
<body>

    <form class="form-card" method="POST" id="form-card" method="POST" action="/save">
        <button class="theme-toggle" id="themeToggle">🌙</button>
        <h1>Credential Entry</h1>

        <div class="section">
            <div class="section-header">
                WiFi Network
                <span class="arrow">▶</span>
            </div>
            <div class="section-content">
                <input type="text" placeholder="SSID" name="wifi-ssid" value="%WIFI_SSID%" required/>
                <input type="password" placeholder="Password" name="wifi-password" value="%WIFI_PASSWORD%" required/>
            </div>
        </div>

        <div class="section">
            <div class="section-header">
                Data Source
                <span class="arrow">▶</span>
            </div>
            <div class="section-content">
                <select id="data-source" name="data-source" required>
                    <option value="dexcom" %DEXCOM%>Dexcom Share</option>
                    <option value="nightscout" %NIGHTSCOUT%>Nightscout</option>
                </select>

                <!-- Card for Dexcom -->
                <div id="dexcom-card" class="card" style="display: none;">
                    <input type="text" placeholder="Username" name="dex-username" value="%DEX_USERNAME%"/>
                    <input type="password" placeholder="Password" name="dex-password" value="%DEX_PASSWORD%"/>
                    <label class="switch">
                        <span>Out of US</span>
                        <input type="checkbox" name="ous" %OUS%/>
                        <span class="slider"></span>
                    </label>
                </div>

                <!-- Card for Nightscout -->
                <div id="nightscout-card" class="card" style="display: none;">
                    <input type="text" placeholder="URL (http://YOUR-URL/)" name="ns-url"value="%NS_URL%"/>
                    <input type="password" placeholder="API Secret" name="ns-secret"value="%NS_SECRET%"/>
                </div>
                <label class="switch">
                    <span>Use Mmol/L</span>
                    <input type="checkbox" name="mmol-l" %MMOL/L%/>
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <div class="section">
            <div class="section-header">
                UI Options
                <span class="arrow">▶</span>
            </div>
            <div class="section-content">
                <label class="switch">
                    <span>12H Time</span>
                    <input type="checkbox" name="12h-time" %12H_TIME%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Enable Trend Arrow</span>
                    <input type="checkbox" name="trend-arrow" %TREND_ARROW%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Enable Delta</span>
                    <input type="checkbox" name="delta" %DELTA%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Enable Timestamp</span>
                    <input type="checkbox" name="timestamp" %TIMESTAMP%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Enable Relative Timestamp</span>
                    <input type="checkbox" name="rel-timestamp" %REL_TIMESTAMP%/>
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <div class="section">
            <div class="section-header">
                Update Options
                <span class="arrow">▶</span>
            </div>
            <div class="section-content">
                <label class="switch">
                    <span>Check for updates</span>
                    <input type="checkbox" name="update-check" %UPDATE_CHECK%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Auto Update</span>
                    <input type="checkbox" name="auto-update" %AUTO_UPDATE%/>
                    <span class="slider"></span>
                </label>
                <label class="switch">
                    <span>Use Beta Channel <span style="color: red; font-weight: bold;">(EXPEREMENTAL)</span></span>
                    <input type="checkbox" name="beta" %BETA%/>
                    <span class="slider"></span>
                </label>
            </div>
        </div>

        <button type="submit" class="submit">Save Configuration</button>
    </form>

<script>
    // Dark mode
    const toggleBtn = document.getElementById('themeToggle');
    const body = document.body;
    if (localStorage.theme === 'dark') {
        body.classList.add('dark');
        toggleBtn.textContent = '☀️';
    }
    toggleBtn.addEventListener('click', (e) => {
        e.preventDefault();
        body.classList.toggle('dark');
        if (body.classList.contains('dark')) {
            localStorage.theme = 'dark';
            toggleBtn.textContent = '☀️';
        } else {
            localStorage.theme = 'light';
            toggleBtn.textContent = '🌙';
        }
    });

    // Collapsible sections
    document.querySelectorAll('.section').forEach(section => {
        const header = section.querySelector('.section-header');
        const content = section.querySelector('.section-content');
        const arrow = section.querySelector('.arrow');
        header.addEventListener('click', () => {
            const isOpen = content.classList.contains('open');
            if (isOpen) {
                content.style.maxHeight = null;
                content.classList.remove('open');
                arrow.classList.remove('open');
            } else {
                content.style.maxHeight = content.scrollHeight + 'px';
                content.classList.add('open');
                arrow.classList.add('open');
            }
        });
    });
    
    document.getElementById("form-card").addEventListener("submit", function() {
        const checkboxes = document.querySelectorAll("input[type='checkbox']");
        
        checkboxes.forEach(function(checkbox) {
            const hiddenInput = document.querySelector(`input[name='${checkbox.name}'][type='hidden']`);
            // If checkbox is unchecked and hidden input doesn't exist, create it
            if (!checkbox.checked && !hiddenInput) {
                const hidden = document.createElement("input");
                hidden.type = "hidden";
                hidden.name = checkbox.name;
                hidden.value = "off";
                document.getElementById("form-card").appendChild(hidden);
            }
        });
    });
    const modeSelect = document.getElementById('data-source');
    const dexcomCard = document.getElementById('dexcom-card');
    const nightscoutCard = document.getElementById('nightscout-card');
    function updateDataSource(){
        dexcomCard.style.display = modeSelect.value === 'dexcom' ? 'block' : 'none';
        nightscoutCard.style.display = modeSelect.value === 'nightscout' ? 'block' : 'none';

        document.querySelectorAll('.section').forEach(section => {
            const content = section.querySelector('.section-content');
            const isOpen = content.classList.contains('open');
            if (isOpen) {
                content.style.maxHeight = content.scrollHeight + 'px';
            }
        });
    }
    updateDataSource()
    modeSelect.addEventListener('change', () => {
        updateDataSource()
    });
</script>

</body>
</html>
)rawliteral";

String finishedHtmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>GlucoView</title>
        <meta name="description" content="">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="stylesheet" href="">
    </head>
    <body>
        The configuration is finished, you can now close this page.
    </body>
</html>
)rawliteral";

String CheckboxValue(bool value){
    if (value) {return "checked";}
    else {return "";}
}

String MaskPassword(String password){
    string output;
    for (char c : password){
        output += "*";
    }
    return output.c_str();
}

String GetMainHtml(Config config){
    String html = mainHtmlPage;
    // WiFi
    html.replace("\%WIFI_SSID%", GetStringValue("wifi-ssid", config));
    html.replace("\%WIFI_PASSWORD%", MaskPassword(GetStringValue("wifi-password", config)));
    // Dexcom
    html.replace("\%DEX_USERNAME%", GetStringValue("dex-username", config));
    html.replace("\%DEX_PASSWORD%", MaskPassword(GetStringValue("dex-password", config)));
    html.replace("\%OUS%", CheckboxValue(GetBooleanValue("ous", config)));
    // Nightscout
    html.replace("\%NS_URL%", GetStringValue("ns-url", config));
    html.replace("\%NS_SECRET%", MaskPassword(GetStringValue("ns-secret", config)));

    html.replace("\%MMOL/L%", CheckboxValue(GetBooleanValue("mmol-l", config)));

    // Dexcom
    if (GetStringValue("data-source", config) == "dexcom"){html.replace("\%DEXCOM%", "selected");}
    else{html.replace("\%DEXCOM%", "");}
    // Nightscout
    if (GetStringValue("data-source", config) == "nightscout"){html.replace("\%NIGHTSCOUT%", "selected");}
    else{html.replace("\%NIGHTSCOUT%", "");}
    
    // Ui Options
    html.replace("\%TREND_ARROW%",  CheckboxValue(GetBooleanValue("trend-arrow", config)));
    html.replace("\%DELTA%", CheckboxValue(GetBooleanValue("delta", config)));
    html.replace("\%12H_TIME%", CheckboxValue(GetBooleanValue("12h-time", config)));
    html.replace("\%TIMESTAMP%", CheckboxValue(GetBooleanValue("timestamp", config)));
    html.replace("\%REL_TIMESTAMP%", CheckboxValue(GetBooleanValue("rel-timestamp", config)));

    // Update Options
    html.replace("\%UPDATE_CHECK%", CheckboxValue(GetBooleanValue("update-check", config)));
    html.replace("\%AUTO_UPDATE%", CheckboxValue(GetBooleanValue("auto-update", config)));
    html.replace("\%BETA%", CheckboxValue(GetBooleanValue("beta", config)));

    return html;
} 

String GetFinishedHtml(){
    return finishedHtmlPage;
}