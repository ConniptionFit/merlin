module.exports = [
  {
    type: 'heading',
    defaultValue: 'Merlin'
  },
  {
    type: 'text',
    defaultValue: 'Configure your Gemini API key and backend server.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'input',
        messageKey: 'GeminiApiKey',
        label: 'Gemini API Key',
        attributes: {
          type: 'text',
          placeholder: 'Your Gemini API key'
        }
      },
      {
        type: 'input',
        messageKey: 'BackendUrl',
        label: 'Backend URL',
        defaultValue: 'http://localhost:8080',
        attributes: {
          type: 'text',
          placeholder: 'http://192.168.1.42:8080'
        }
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save Settings'
  }
];
