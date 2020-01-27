package in3;

import java.util.HashMap;

import org.json.JSONObject;

public class IN3MockBuilder {
    IN3 client;

	public void createNewClient() {
        client = IN3.forChain(Chain.GOERLI);
	}

    public void buildTransport(String[][] fileNameTuples) {
        IN3MockTransport newtransport = new IN3MockTransport();

        // TODO Consider on using Apache Commons Pair
        for (String[] fileNameTuple: fileNameTuples) {
            newtransport.setMockResponse(fileNameTuple[0], fileNameTuple[1]);
        }

        client.setTransport(newtransport);
    }

    public void buildConfig() {
        HashMap<String, Object> nodeConfig = new HashMap<String, Object>();
        nodeConfig.put("needsUpdate", false);
        nodeConfig.put("contract", "0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f");
        nodeConfig.put("registryId", "0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb");

        HashMap<String, Object> nodes = new HashMap<String, Object>();
        nodes.put("0x1", nodeConfig.clone());
        nodes.put("0x5", nodeConfig.clone());
        nodes.put("0x2a", nodeConfig.clone());

        HashMap<String, Object> configMap = new HashMap<String, Object>();

        configMap.put("requestCount", 1);
        configMap.put("autoUpdateList", false);
        configMap.put("proof", Proof.none);
        configMap.put("autoUpdateList", false);
        configMap.put("maxAttempts", 1);
        configMap.put("signatureCount", 0);
        configMap.put("nodes", nodes);

        JSONObject configJson = new JSONObject(configMap);
        client.setConfig(configJson.toString());
    }

	public IN3 constructClient(String[][] fileNameTuples) {
		createNewClient();
		buildTransport(fileNameTuples);
		buildConfig();
        return client;
    }
}
