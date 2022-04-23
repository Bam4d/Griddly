class GDYHistory {
  constructor(maxHistory) {
    this.maxHistory = maxHistory;
  }

  saveGDY(envName, gdyString) {
    let envDataString = window.localStorage.getItem(envName);

    let envData;
    if (!envDataString) {
      envData = {
        history: [],
      };
    } else {
      envData = JSON.parse(envDataString);
    }

    envData.history.push(gdyString);

    if (envData.history.length >= this.maxHistory) {
      envData.history.shift();
    }

    window.localStorage.setItem(envName, JSON.stringify(envData));
  } 

  loadGDY(envName) {
    const envDataString = window.localStorage.getItem(envName);
    if (!envDataString) {
      return undefined;
    }

    const envData = JSON.parse(envDataString);

    return envData.history[envData.history.length-1];
  }
}

export default GDYHistory;
