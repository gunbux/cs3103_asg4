import React, { useState, ChangeEvent } from 'react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Textarea } from '@/components/ui/textarea';

interface ReportItem {
  department: string;
  count: number;
}

function App() {
  const [file, setFile] = useState<File | null>(null);
  const [departmentCode, setDepartmentCode] = useState<string>('');
  const [subject, setSubject] = useState<string>('');
  const [body, setBody] = useState<string>('');
  const [report, setReport] = useState<ReportItem[]>([]);

  const handleFileUpload = (e: ChangeEvent<HTMLInputElement>) => {
    if (e.target.files) {
      setFile(e.target.files[0]);
    }
  };

  const handleSendEmails = () => {
    // Integrate with the backend to send emails
    // Process file, subject, body, and departmentCode
    // Generate report and set it using setReport()
  };

  return (
    <div className="flex items-center justify-center min-h-screen w-screen bg-gray-100">
      <div className="container max-w-lg mx-auto p-6 bg-white shadow-lg rounded-md">
        <h1 className="text-2xl font-bold mb-4 text-center">Smart Mailer</h1>

        <div className="mb-4">
          <label className="block font-semibold mb-2">Upload CSV File</label>
          <Input type="file" onChange={handleFileUpload} />
        </div>

        <div className="mb-4">
          <label className="block font-semibold mb-2">Department Code</label>
          <Input
            type="text"
            value={departmentCode}
            onChange={(e: ChangeEvent<HTMLInputElement>) => setDepartmentCode(e.target.value)}
            placeholder="Enter department code or 'all'"
          />
        </div>

        <div className="mb-4">
          <label className="block font-semibold mb-2">Email Subject</label>
          <Input
            type="text"
            value={subject}
            onChange={(e: ChangeEvent<HTMLInputElement>) => setSubject(e.target.value)}
            placeholder="Enter email subject"
          />
        </div>

        <div className="mb-4">
          <label className="block font-semibold mb-2">Email Body (HTML)</label>
          <Textarea
            value={body}
            onChange={(e: ChangeEvent<HTMLTextAreaElement>) => setBody(e.target.value)}
            placeholder="Enter email body with placeholders (#name#, #department#)"
            rows={6}
          />
        </div>

        <div className="mb-4">
          <Button onClick={handleSendEmails} className="bg-blue-500 text-white w-full">
            Send Emails
          </Button>
        </div>

        <div className="mt-8">
          <h2 className="text-xl font-semibold mb-4 text-center">Email Sent Report</h2>
          {report.length > 0 ? (
            <ul className="list-disc list-inside">
              {report.map((item, index) => (
                <li key={index}>
                  {item.department}: {item.count} emails sent
                </li>
              ))}
            </ul>
          ) : (
            <p className="text-center">No report available</p>
          )}
        </div>
      </div>
    </div>
  );
}

export default App;
