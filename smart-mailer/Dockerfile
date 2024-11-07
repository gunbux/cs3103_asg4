# Use Node.js to build the frontend
FROM node:18 AS builder

# Set the working directory
WORKDIR /app

# Copy the package.json and install dependencies
COPY package.json yarn.lock ./
RUN yarn install

# Copy the rest of the application code
COPY . .

# Build the production files
RUN yarn build

# Use an Nginx image to serve the built files
FROM nginx:alpine

# Copy the built files from the previous stage to the Nginx directory
COPY --from=builder /app/build /usr/share/nginx/html

# Expose port 80 to access the frontend
EXPOSE 80

# Start Nginx server
CMD ["nginx", "-g", "daemon off;"]
