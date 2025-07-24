#include <WiFi.h>
#include "config/config_manager.h"

String CheckboxValue(bool value){
    if (value) {return "checked";}
    else {return "";}
}

void PrintMainHtml(WiFiClient &client, Config config){

    client.println("<!DOCTYPE html>");
    client.println("<html lang=\"en\">");
    client.println("<head>");
    client.println("  <meta charset=\"UTF-8\" />");
    client.println("  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>");
    client.println("  <title>GlucoView</title>");
    client.println("  <style>");
    client.println("    body {");
    client.println("      font-family: Arial, sans-serif;");
    client.println("      background-color: #f5f7fa;");
    client.println("      padding: 2rem;");
    client.println("    }");
    client.println("");
    client.println("    .container {");
    client.println("      max-width: 50%;");
    client.println("      max-height: 75%;");
    client.println("      margin: auto;");
    client.println("      background: white;");
    client.println("      padding: 2rem;");
    client.println("      border-radius: 10px;");
    client.println("      box-shadow: 0 4px 10px rgba(0,0,0,0.1);");
    client.println("    }");
    client.println("");
    client.println("    h2 {");
    client.println("      text-align: center;");
    client.println("      margin-bottom: 1.5rem;");
    client.println("    }");
    client.println("");
    client.println("    label{");
    client.println("      margin-top: 1rem;");
    client.println("      font-weight: bold;");
    client.println("    }");
    client.println("");
    client.println("    .blocklabel {");
    client.println("      display: block;");
    client.println("    }");
    client.println("");
    client.println("    input[type=\"text\"],");
    client.println("    input[type=\"password\"] {");
    client.println("      width: 100%;");
    client.println("      padding: 0.5rem;");
    client.println("      margin-top: 0.25rem;");
    client.println("      border: 1px solid #ccc;");
    client.println("      border-radius: 5px;");
    client.println("    }");
    client.println("");
    client.println("    button {");
    client.println("      width: 100%;");
    client.println("      margin-top: 2rem;");
    client.println("      padding: 0.75rem;");
    client.println("      background-color: #4CAF50;");
    client.println("      color: white;");
    client.println("      border: none;");
    client.println("      border-radius: 5px;");
    client.println("      font-size: 1rem;");
    client.println("      cursor: pointer;");
    client.println("    }");
    client.println("");
    client.println("    button:hover {");
    client.println("      background-color: #45a049;");
    client.println("    }");
    client.println("");
    client.println("  </style>");
    client.println("</head>");
    client.println("<body>");
    client.println("  <div class=\"container\">");
    client.println("    <h2>Configure GlucoView</h2>");
    client.println("    <form id=\"coolForm\">");
    client.println("      <label for=\"ssid\" class=\"blocklabel\">WiFi SSID</label>");
    client.println("      <input type=\"text\" id=\"ssid\" name=\"wifi-ssid\" value=\""+get<String>(config["wifi-ssid"])+"\" required>");
    client.println("");
    client.println("      <label for=\"wifiPassword\" class=\"blocklabel\">WiFi Password</label>");
    client.println("      <input type=\"password\" id=\"wifiPassword\" name=\"wifi-password\" value=\""+get<String>(config["wifi-password"])+"\" required>");
    client.println("");
    client.println("      <label for=\"dexcomUser\" class=\"blocklabel\">Dexcom Username</label>");
    client.println("      <input type=\"text\" id=\"dexcomUser\" name=\"dex-username\" value=\""+get<String>(config["dex-username"])+"\" required>");
    client.println("");
    client.println("      <label for=\"dexcomPass\" class=\"blocklabel\">Dexcom Password</label>");
    client.println("      <input type=\"password\" id=\"dexcomPass\" name=\"dex-password\" value=\""+get<String>(config["dex-password"])+"\" required>");
    client.println("");
    client.println("      <label class=\"blocklabel\">12H Reading timestamp:</label>");
    client.println("      <input type=\"checkbox\" name=\"12h-time\" id=\"twelveHourTime\" value=\"on\" "+CheckboxValue(get<bool>(config["12h-time"]))+">");
    client.println("      <label id=\"twelveHourTimeExample\">Example: 20:55</label>");
    client.println("");
    client.println("      <button type=\"submit\">Save Configuration</button>");
    client.println("    </form>");
    client.println("  </div>");
    client.println("</body>");
    client.println("</html>");
    client.println("<script>");
    client.println("  var twelveHourTimeCheckbox = document.getElementById(\"twelveHourTime\");");
    client.println("  var twelveHourTimeExampleLabel = document.getElementById(\"twelveHourTimeExample\");");
    client.println("");
    client.println("  function updateTwelveHourTimeExample(){");
    client.println("    if (twelveHourTimeCheckbox.checked){twelveHourTimeExampleLabel.textContent = \"Example: 9:55 PM\";}");
    client.println("    else {twelveHourTimeExampleLabel.textContent = \"Example: 20:55\";}");
    client.println("  }");
    client.println("  updateTwelveHourTimeExample();");
    client.println("  twelveHourTimeCheckbox.onchange = updateTwelveHourTimeExample;");
    client.println("");
    client.println("");
    client.println("  document.getElementById(\"coolForm\").addEventListener(\"submit\", function() {");
    client.println("    const checkboxes = document.querySelectorAll(\"input[type='checkbox']\");");
    client.println("");
    client.println("    checkboxes.forEach(function(checkbox) {");
    client.println("        const hiddenInput = document.querySelector(`input[name='${checkbox.name}'][type='hidden']`);");
    client.println("");
    client.println("        // If checkbox is unchecked and hidden input doesn't exist, create it");
    client.println("        if (!checkbox.checked && !hiddenInput) {");
    client.println("            const hidden = document.createElement(\"input\");");
    client.println("            hidden.type = \"hidden\";");
    client.println("            hidden.name = checkbox.name;");
    client.println("            hidden.value = \"off\";");
    client.println("            document.getElementById(\"coolForm\").appendChild(hidden);");
    client.println("        }");
    client.println("    });");
    client.println("});");
    client.println("</script>");
}

void PrintFinishedHtml(WiFiClient &client){
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("    <head>");
    client.println("        <meta charset=\"utf-8\">");
    client.println("        <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">");
    client.println("        <title>GlucoView</title>");
    client.println("        <meta name=\"description\" content=\"\">");
    client.println("        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("        <link rel=\"stylesheet\" href=\"\">");
    client.println("    </head>");
    client.println("    <body>");
    client.println("        The configuration is finished, you can now close this page.");
    client.println("    </body>");
    client.println("</html>");
}