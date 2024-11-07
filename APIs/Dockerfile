# Use the official Node.js image as the base image
FROM node:18

# Set the working directory inside the container
WORKDIR /usr/src/app

# Copy the package.json and package-lock.json (if exists) from the APIs folder
COPY ./APIs/package*.json ./

# Install the app dependencies
RUN npm install

# Copy the entire APIs folder content, including server.js and other required files
COPY ./APIs/ ./

# Expose port 3000 for the Express app to be accessible externally
EXPOSE 3000

# Command to run the Express app
CMD ["node", "server.js"]
